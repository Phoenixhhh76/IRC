#include "Server.hpp"
#include "acceptClients.hpp"
#include "Parser.hpp"
#include "Replies.hpp"
#include "CommandInit.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <utility>   // std::make_pair
#include <sstream>
#include <ctime>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server(int port, const std::string& password)
    : _listener(port), _servername("ft_irc"), _password(password), _botNick("ft_irc_Bot"), _botFd(-1) {
    struct pollfd p;
    p.fd = _listener.getFd();
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);
    initCommands();
    initBot();
}
Server::~Server() {
    // Clean up any remaining heap-allocated Client objects
    // 先清理仍存在的 DCC 會話，避免 FD 泄漏
    for (std::map<int, DccSession>::iterator ds = _dccByFd.begin(); ds != _dccByFd.end(); ++ds) {
        if (ds->second.ofs && ds->second.ofs->is_open()) ds->second.ofs->close();
        if (ds->second.sockFd >= 0) ::close(ds->second.sockFd);
    }
    _dccByFd.clear();

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second) {
            it->second->closeNow();
            delete it->second;
        }
    }
    _clients.clear();
};
// ========== helpers ==========

void Server::enableWriteForFd(int fd) {
    for (size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events |= POLLOUT;
            return;
        }
    }
}

bool Server::isNickInUse(const std::string& nick) const {
    return _nick2fd.find(toLower(nick)) != _nick2fd.end();
}

void Server::takeNick(Client& cl, const std::string& newNick) {
    // 移除舊 nick
    if (cl.hasNick()) {
        std::string oldLower = toLower(cl.getNick());
        std::map<std::string,int>::iterator it = _nick2fd.find(oldLower);
        if (it != _nick2fd.end() && it->second == cl.fd())
            _nick2fd.erase(it);
    }
    // 設置新 nick（保留原大小寫給顯示）
    cl.setNick(newNick);
    _nick2fd[toLower(newNick)] = cl.fd();
}

void Server::sendWelcome(Client& cl) {
    cl.sendLine(RPL_WELCOME(_servername, cl.getNick()));
    enableWriteForFd(cl.fd());
}

const std::string& Server::serverName() const { return _servername; }
// ========== dispatcher for IRC clientcommands ==========
void Server::initCommands() {
    registerAllCommands(_cmds, *this);
}

void Server::handleIrcMessage(Client& cl, const IrcMessage& m) {
    const std::string& cmd = m.command;

    std::cout << "[DEBUG] Server received command: '" << cmd << "' from fd=" << cl.fd() << std::endl;

    if (!cl.registered()) {
        if (!cl.hasPass() && (cmd == "NICK" || cmd == "USER")) { 
            //if the client has not passed the password, and the command is NICK or USER, send 464
            cl.sendLine(ERR_PASSWDMISMATCH(_servername));
            enableWriteForFd(cl.fd());
            return;
        }
        if (cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "PING" && cmd != "CAP" && cmd != "QUIT") {
            //if the command is not PASS, NICK, USER, PING, CAP, or QUIT, send 451
            cl.sendLine(ERR_NOTREGISTERED(_servername));
            enableWriteForFd(cl.fd());
            std::cout << "[DEBUG] Command '" << cmd << "' rejected (not registered), sent 451" << std::endl;
            return;
        }
    }

    if (!_cmds.dispatch(cmd, *this, cl, m)) {
        std::cout << "[DEBUG] Unknown command: '" << cmd << "'" << std::endl;
        // 未知指令（可選）
        // cl.sendLine(ERR_UNKNOWNCOMMAND(_servername, cmd));
        // enableWriteForFd(cl.fd());
    }
}

void Server::run() {
    std::cout << "Server listening on port " << _listener.getPort() << std::endl;

    for (;;) {
        if (stopRequested()) break;
        int ready = ::poll(&_pfds[0], _pfds.size(), -1);
        if (ready < 0) {
            if (errno == EINTR) {
                if (stopRequested()) break; // graceful stop on signal
                continue;
            }
            throw std::runtime_error("poll: " + std::string(std::strerror(errno)));
        }

        // 1) 只處理 poll 這一輪「原本就存在」的條目，避免 push_back 期間重配
        const size_t originalCount = _pfds.size();

        // 暫存：本輪新增的 client 與要關閉的 index
        std::vector<int> toAddFds;
        std::vector<size_t> toCloseIdx;

        // 處理所有 poll 事件
        for (size_t i = 0; i < originalCount; ++i) {
            if (_pfds[i].revents) {
                processPollEvent(i, toAddFds, toCloseIdx);
            }
        }

        // 2) 關閉/移除（倒序處理 index，避免位移）
        for (size_t k = 0; k < toCloseIdx.size(); ++k) {
            size_t idx = toCloseIdx[toCloseIdx.size() - 1 - k];
            closeClient(idx);
        }

        // 3) 追加新連線
        for (size_t j = 0; j < toAddFds.size(); ++j) {
            addNewClient(toAddFds[j]);
        }
    }
}

//--------channel tool

static std::map<std::string, Channel>::iterator
ensureChannel(std::map<std::string, Channel>& chans, const std::string& ch) {
    std::map<std::string, Channel>::iterator it = chans.find(ch);
    if (it == chans.end()) {
        // Channel 只有帶名建構子，所以用 insert 建
        it = chans.insert(std::make_pair(ch, Channel(ch))).first;
    }
    return it;
}

// --- 1) 廣播到頻道 ---
void Server::broadcastToChannel(const std::string& ch,
                                const std::string& line,
                                int except_fd)
{
    const std::string key = normalizeChannelName(ch);
    std::map<std::string, Channel>::iterator it = _channels.find(key);
    if (it == _channels.end()) return;

    const std::set<int>& mem = it->second.members();
    for (std::set<int>::const_iterator m = mem.begin(); m != mem.end(); ++m) {
        int fd = *m;
        if (fd == except_fd) continue;
        std::map<int, Client*>::iterator c = _clients.find(fd);
        if (c != _clients.end() && c->second) {
            c->second->sendLine(line);
            enableWriteForFd(fd);
        }
    }
}


bool Server::addClientToChannel(const std::string& ch, int fd)
{
    const std::string key = normalizeChannelName(ch);
    // 取得/建立頻道（使用規範化名稱作為鍵）
    std::map<std::string, Channel>::iterator it = ensureChannel(_channels, key);
    Channel& C = it->second;

    // 寫入 Channel
    bool inserted = C.addClient(fd);

    // 同步 Client 的 channel 清單
    if (inserted) {
        std::map<int, Client*>::iterator cit = _clients.find(fd);
        if (cit != _clients.end() && cit->second) {
            cit->second->addChannel(key);
        }
    }
    return inserted;
}


bool Server::removeClientFromChannel(const std::string& ch, int fd)
{
    const std::string key = normalizeChannelName(ch);
    std::map<std::string, Channel>::iterator it = _channels.find(key);
    if (it == _channels.end()) return false;

    Channel& C = it->second;
    if (!C.hasClient(fd)) return false;

    // 從 Channel 移除
    C.removeClient(fd);

    // 從 Client 的 channel 清單移除
    std::map<int, Client*>::iterator cit = _clients.find(fd);
    if (cit != _clients.end() && cit->second) {
        cit->second->removeChannel(key);
    }

    // 若頻道空了，立即刪除（符合 IRC：空頻道即銷毀，模式/主題/邀請等重置）
    if (C.members().empty()) {
        std::cout << "[DEBUG] Channel " << key << " became empty, removing" << std::endl;
        _channels.erase(it);
    }
    return true;
}

bool Server::isChannelMember(const std::string& ch, int fd) const
{
    const std::string key = normalizeChannelName(ch);
    std::map<std::string, Channel>::const_iterator it = _channels.find(key);
    if (it == _channels.end()) return false;
    return it->second.hasClient(fd);
}

bool Server::channelExists(const std::string& ch) const {
    const std::string key = normalizeChannelName(ch);
    return _channels.find(key) != _channels.end();
}


void Server::sendToNick(const std::string& nick, const std::string& line) {
    std::map<std::string,int>::const_iterator it = _nick2fd.find(toLower(nick));
    if (it == _nick2fd.end()) return;
    int fd = it->second;
    std::map<int, Client*>::iterator c = _clients.find(fd);
    if (c == _clients.end() || c->second == NULL) return;
    c->second->sendLine(line);
    enableWriteForFd(fd);
}

// --- 密碼與操作員相關 ---
bool Server::checkPassword(const std::string& pwd) const {
    return _password == pwd;
}

bool Server::isOperator(const std::string& nick) const {
    return _operators.find(nick) != _operators.end();
}

void Server::addOperator(const std::string& nick) {
    _operators.insert(nick);
}

int Server::getFdByNick(const std::string& nick) const {
    std::map<std::string,int>::const_iterator it = _nick2fd.find(toLower(nick));
    if (it == _nick2fd.end()) return -1;
    return it->second;
}

Channel& Server::getChannel(const std::string& ch) {
    const std::string key = normalizeChannelName(ch);
    std::map<std::string, Channel>::iterator it = _channels.find(key);
    if (it == _channels.end()) {
        // 頻道不存在，建立新的
        std::cout << "[DEBUG] Creating new channel: " << key << std::endl;
        std::pair<std::map<std::string, Channel>::iterator, bool> result =
            _channels.insert(std::make_pair(key, Channel(key)));
        it = result.first;
        std::cout << "[DEBUG] Channel created with mode: " << (it->second.isInviteOnly() ? "invite-only" : "public") << std::endl;
    } else {
        std::cout << "[DEBUG] Found existing channel: " << key << std::endl;
        std::cout << "[DEBUG] Channel mode: " << (it->second.isInviteOnly() ? "invite-only" : "public") << std::endl;
    }
    return it->second;
}

bool Server::isChannelOperator(const std::string& ch, int fd) const {
    const std::string key = normalizeChannelName(ch);
    std::map<std::string, Channel>::const_iterator it = _channels.find(key);
    if (it == _channels.end()) return false;
    return it->second.isOperator(fd);
}


// ========== run() 輔助函數 ==========

void Server::processPollEvent(size_t idx, std::vector<int>& toAddFds, std::vector<size_t>& toCloseIdx) {
    struct pollfd cur = _pfds[idx]; // 拷貝成局部，不拿參考，避免失效

    // A) Listener 可讀 → 先把新連線記到暫存陣列，迴圈結束後再 push_back
    if ((cur.revents & POLLIN) && cur.fd == _listener.getFd()) {
        std::vector<std::pair<int, std::string> > news = acceptClients(_listener.getFd());
        for (size_t j = 0; j < news.size(); ++j) {
            toAddFds.push_back(news[j].first); // Save the fd
            // Store the IP address to be used in addNewClient
            if (_clients.find(news[j].first) == _clients.end()) {
                Client* newClient = new Client(news[j].first);
                newClient->setHost(news[j].second); // Set the client's host to their IP
                _clients[news[j].first] = newClient;
                struct pollfd np;
                np.fd = news[j].first;
                np.events = POLLIN;
                np.revents = 0;
                _pfds.push_back(np);
            }
        }
    }
    // B) Client 可讀
    else if (cur.revents & POLLIN) {
        if (_dccByFd.find(cur.fd) != _dccByFd.end()) {
            processDccPollEvent(idx, toCloseIdx);
            return;
        }
        int fd = cur.fd;
        std::map<int, Client*>::iterator it = _clients.find(fd);
        if (it == _clients.end() || it->second == NULL) {
            toCloseIdx.push_back(idx);
            return;
        }
        Client *cl = it->second;

        if (!cl->readFromSocket()) { // 對端關閉或讀錯
            std::cout << "[DEBUG] Marking client idx=" << idx << " fd=" << fd << " for close" << std::endl;
            toCloseIdx.push_back(idx);
            return;
        }

        std::string line;
        while (cl->popLine(line)) {
            try {
                IrcMessage m = parseLine(line);
                handleIrcMessage(*cl, m);
            } catch (const std::exception& e) {
                toCloseIdx.push_back(idx);
                return;
            } catch (...) {
                toCloseIdx.push_back(idx);
                return;
            }
        }
    }
    // C) Client 可寫
    else if (cur.revents & POLLOUT) {
        if (_dccByFd.find(cur.fd) != _dccByFd.end()) {
            processDccPollEvent(idx, toCloseIdx);
            return;
        }
        int fd = cur.fd;
        std::map<int, Client*>::iterator it = _clients.find(fd);
        if (it == _clients.end() || it->second == NULL) {
            toCloseIdx.push_back(idx);
            return;
        }
        Client *cl = it->second;

        try {
            cl->flushOutbuf();
        } catch (const std::exception& e) {
            toCloseIdx.push_back(idx);
            return;
        } catch (...) {
            toCloseIdx.push_back(idx);
            return;
        }
        if (cl->isOutbufEmpty()) {
            _pfds[idx].events &= ~POLLOUT; // 送完就關掉避免 busy loop
        }
    }

    // D) 錯誤/關閉
    if (cur.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        if (_dccByFd.find(cur.fd) != _dccByFd.end()) {
            processDccPollEvent(idx, toCloseIdx);
        } else {
            toCloseIdx.push_back(idx);
        }
    }
}

void Server::closeClient(size_t idx) {
    std::cout << "[DEBUG] closeClient called for idx=" << idx << std::endl;
    if (idx >= _pfds.size()) {
        std::cout << "[DEBUG] WARNING: idx >= _pfds.size() (" << idx << " >= " << _pfds.size() << ")" << std::endl;
        return;
    }
    int fd = _pfds[idx].fd;
    std::cout << "[DEBUG] Closing client fd=" << fd << std::endl;

    // DCC socket 關閉處理
    std::map<int, DccSession>::iterator ds = _dccByFd.find(fd);
    if (ds != _dccByFd.end()) {
        DccSession& sess = ds->second;
        if (sess.ofs && sess.ofs->is_open()) sess.ofs->close();
        ::close(sess.sockFd);
        std::map<int, Client*>::iterator owner = _clients.find(sess.ownerFd);
        if (owner != _clients.end() && owner->second) {
            std::string note = ":" + _servername + " NOTICE " + owner->second->getNick() + " :DCC ";
            note += sess.finished ? "receive complete: " : "receive aborted: ";
            note += sess.savePath;
            owner->second->sendLine(note);
            enableWriteForFd(owner->first);
        }
        _dccByFd.erase(ds);
        if (idx < _pfds.size() - 1) std::swap(_pfds[idx], _pfds.back());
        _pfds.pop_back();
        return;
    }

    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        if (it->second && it->second->hasNick()) {
            std::string lower = toLower(it->second->getNick());
            std::map<std::string,int>::iterator nit = _nick2fd.find(lower);
            if (nit != _nick2fd.end() && nit->second == fd) _nick2fd.erase(nit);
        }
        if (it->second) {
            it->second->closeNow();
            delete it->second;
        }
        _clients.erase(it);
    } else {
        ::close(fd);
    }
    if (idx < _pfds.size() - 1) std::swap(_pfds[idx], _pfds.back());
    _pfds.pop_back();
}

void Server::addNewClient(int fd) {
    // Skip if client was already added in processPollEvent
    if (_clients.find(fd) != _clients.end()) {
        return;
    }

    Client* newClient = new Client(fd);
    newClient->setHost("unknown"); // Fallback host if we don't have the IP
    _clients.insert(std::make_pair(fd, newClient));
    struct pollfd np;
    np.fd = fd;
    np.events = POLLIN;
    np.revents = 0;
    _pfds.push_back(np);
}


Client* Server::getClientByFd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    return (it == _clients.end()) ? NULL : it->second;
}

const Client* Server::getClientByFd(int fd) const {
    std::map<int, Client*>::const_iterator it = _clients.find(fd);
    return (it == _clients.end()) ? NULL : it->second;
}
