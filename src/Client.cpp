#include "Client.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstring>      // std::strerror
#include <unistd.h>     // read, write, close
#include <sys/types.h>
#include <sys/socket.h> // recv, send

Client::Client(int fd)
: _fd(fd), _inbuf(), _outbuf(),
  _nick(), _user(), _realname(), _host("localhost"),
  _isAuthenticated(false), _hasPass(false), _hasNick(false), _hasUser(false), _registered(false)
{}

Client::~Client() {
    // 讓解構子安全地收尾；若已經 closeNow() 過，_fd 會是 -1，這裡不會再次關閉
    closeNow();
}

// Copy constructor and copy assignment are intentionally not implemented.
// Client is non-copyable in this C++98 codebase because it owns a socket fd.

void Client::closeNow() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1; // 避免二次關閉
    }
}
int Client::fd() const { return _fd; }

bool Client::readFromSocket() {
    char buf[4096];
    for (;;) {
        ssize_t n = ::recv(_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            _inbuf.append(buf, buf + n);
            // 繼續讀，直到 EAGAIN（edge/level 觸發都安全）
            continue;
        }
        if (n == 0) {
            // 對端關閉
            return false;
        }
        // n < 0
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 本輪讀完了
            return true;
        }
        if (errno == EINTR) {
            // 被信號打斷，重試
            continue;
        }
        // 其他錯誤：建議回 false 交由上層清理（也可選擇丟例外）
        return false;
    }
}

bool Client::popLine(std::string& out) {
    // IRC 正式以 "\r\n" 結尾；容忍只有 '\n' 的工具（例如某些測試）
    std::string::size_type pos = _inbuf.find("\r\n");
    if (pos != std::string::npos) {
        out.assign(_inbuf, 0, pos);
        _inbuf.erase(0, pos + 2);
        return true;
    }
    // 寬鬆：僅 '\n'
    std::string::size_type lf = _inbuf.find('\n');
    if (lf != std::string::npos) {
        out.assign(_inbuf, 0, lf);
        _inbuf.erase(0, lf + 1);
        if (!out.empty() && out[out.size() - 1] == '\r')
            out.erase(out.size() - 1);
        return true;
    }
    return false;
}

void Client::flushOutbuf() {
    while (!_outbuf.empty()) {
        ssize_t n = ::send(_fd, _outbuf.data(), _outbuf.size(), 0);
        if (n > 0) {
            _outbuf.erase(0, static_cast<std::string::size_type>(n));
            // 繼續嘗試送更多；直到送不動
            continue;
        }
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 目前不可寫：由 poll 啟用 POLLOUT，再等下一輪
                return;
            }
            if (errno == EINTR) {
                // 被信號打斷，重試
                continue;
            }
            // 其他錯誤：交由外層處理（也可改成 return false）
            throw std::runtime_error("send: " + std::string(std::strerror(errno)));
        }
        // n == 0 幾乎不會發生（對 TCP send 而言），保險起見 break
        break;
    }
}

bool Client::isOutbufEmpty() const {
    return _outbuf.empty();
}

void Client::sendLine(const std::string& line) {
    _outbuf.append(line);
    _outbuf.append("\r\n");
}

void Client::appendRaw(const std::string& data) {
    _outbuf.append(data);
}

void Client::setPassOk() { _hasPass = true; }

const std::string Client::getNick() const {
    return _nick;
}

void Client::setNick(const std::string& nickname) {
    _nick = nickname;
    _hasNick = true;
}

const std::string Client::getUser() const {
    return _user;
}

void Client::setUser(const std::string& username,  const std::string& real) {
    _user = username;
    _realname = real;
    _hasUser = true;
}

const std::string& Client::getHost() const { return _host; }
void Client::setHost(const std::string& h) {
    // Guard against empty host strings; Irssi expects host to be non-null/non-empty
    if (h.empty()) {
        _host = "unknown";
    } else {
        _host = h;
    }
}
const std::string& Client::getRealname() const { return _realname; }

std::string Client::getFullPrefix() const {
    return buildPrefix(_nick, _user, _host);
}

bool Client::hasNick() const { return _hasNick; }
bool Client::hasUser() const { return _hasUser; }
bool Client::hasPass() const { return _hasPass; }
bool Client::registered() const { return _registered; }

// Just check if NIck and User is done
bool Client::tryFinishRegister() {
    // 确保已经设置了密码，并且是在NICK和USER之前设置的
    if (!_registered && _hasPass && _hasNick && _hasUser) {
        _registered = true;
        return true;
    }
    return false;
}

bool Client::isAuthenticated() const {
    return _isAuthenticated;
}

void Client::setAuthenticated(bool auth) {
    _isAuthenticated = auth;
}

// ---- for channel ----
void Client::addChannel(const std::string& ch) {
    _channels.insert(normalizeChannelName(ch));
}

void Client::removeChannel(const std::string& ch) {
    _channels.erase(normalizeChannelName(ch));
}

bool Client::inChannel(const std::string& ch) const {
    return _channels.find(normalizeChannelName(ch)) != _channels.end();
}

const std::set<std::string>& Client::channels() const {
    return _channels;
}