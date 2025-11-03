#include "Server.hpp"
#include "bonus/Dcc.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <sstream>

// DccSession member function definitions
DccSession::DccSession() : sockFd(-1), ownerFd(-1), ofs(NULL), connecting(false), finished(false) {}

DccSession::DccSession(const DccSession& other)
    : sockFd(other.sockFd), ownerFd(other.ownerFd),
      filename(other.filename), savePath(other.savePath),
      connecting(other.connecting), finished(other.finished) {
    // Transfer ofstream pointer ownership, not copy
    ofs = other.ofs;
    // Set original object's pointer to NULL to prevent double deletion
    const_cast<DccSession&>(other).ofs = NULL;
}

DccSession& DccSession::operator=(const DccSession& other) {
    if (this != &other) {
        // Clean up current resources
        if (ofs) {
            if (ofs->is_open()) ofs->close();
            delete ofs;
        }

        sockFd = other.sockFd;
        ownerFd = other.ownerFd;
        filename = other.filename;
        savePath = other.savePath;
        connecting = other.connecting;
        finished = other.finished;

        // Transfer ofstream pointer ownership
        ofs = other.ofs;
        const_cast<DccSession&>(other).ofs = NULL;
    }
    return *this;
}

DccSession::~DccSession() {
    if (ofs) {
        if (ofs->is_open()) ofs->close();
        delete ofs;
        ofs = NULL;
    }
    // Note: don't close sockFd here, it's managed uniformly by Server
}

void Server::handleDccSend(int fd, const IrcMessage& m) {
    if (m.params.size() < 4) {
        return;
    }

    const std::string filename = m.params[1];
    const std::string ip = m.params[2];
    const int port = std::atoi(m.params[3].c_str());

    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end() || it->second == NULL) return;

    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC socket error");
        enableWriteForFd(fd);
        return;
    }
    if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
        ::close(s);
        it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC set nonblock failed");
        enableWriteForFd(fd);
        return;
    }
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        ::close(s);
        it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC invalid IP");
        enableWriteForFd(fd);
        return;
    }

    int rc = ::connect(s, (struct sockaddr*)&addr, sizeof(addr));
    bool connecting = false;
    if (rc < 0) {
        if (errno == EINPROGRESS) {
            connecting = true;
        } else {
            ::close(s);
            it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC connect failed");
            enableWriteForFd(fd);
            return;
        }
    }

    std::ostringstream path;
    path << "/tmp/ftirc_" << std::time(NULL) << "_" << filename;

    // Create a temporary object
    DccSession sess;
    sess.sockFd = s;
    sess.ownerFd = fd;
    sess.filename = filename;
    sess.savePath = path.str();
    sess.connecting = connecting;
    sess.finished = false;
    sess.ofs = new std::ofstream(path.str().c_str(), std::ios::out | std::ios::binary);

    if (!sess.ofs->is_open()) {
        delete sess.ofs;
        sess.ofs = NULL;
        ::close(s);
        it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC open file failed");
        enableWriteForFd(fd);
        return;
    }

    // Use insert to avoid copy issues - but this still has problems in C++98
    // We need to restore public copy constructor, but ensure correct resource management
    _dccByFd[s] = sess;

    // Prevent destructor from deleting resources, as map now owns them
    sess.ofs = NULL;

    struct pollfd np;
    np.fd = s;
    np.events = connecting ? (POLLIN | POLLOUT) : POLLIN;
    np.revents = 0;
    _pfds.push_back(np);

    it->second->sendLine(":" + _servername + " NOTICE " + it->second->getNick() + " :DCC receiving to " + _dccByFd[s].savePath);
    enableWriteForFd(fd);
    std::cout << "[DCC] start recv file=" << filename << " from " << ip << ":" << port << " -> " << _dccByFd[s].savePath << std::endl;
}

void Server::processDccPollEvent(size_t idx, std::vector<size_t>& toCloseIdx) {
    if (idx >= _pfds.size()) return;
    struct pollfd cur = _pfds[idx];
    std::map<int, DccSession>::iterator sit = _dccByFd.find(cur.fd);
    if (sit == _dccByFd.end()) return;
    DccSession &sess = sit->second;

    if (sess.connecting && (cur.revents & POLLOUT)) {
        int err = 0; socklen_t len = sizeof(err);
        if (getsockopt(sess.sockFd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
            sess.connecting = false;
            _pfds[idx].events &= ~POLLOUT;
            _pfds[idx].events |= POLLIN;
        } else {
            toCloseIdx.push_back(idx);
            return;
        }
    }

    if (cur.revents & POLLIN) {
        char buf[8192];
        for (;;) {
            ssize_t n = ::recv(sess.sockFd, buf, sizeof(buf), 0);
            if (n > 0) {
                sess.ofs->write(buf, n);
                if (!*sess.ofs) {
                    toCloseIdx.push_back(idx);
                    return;
                }
                continue;
            }
            if (n == 0) {
                sess.finished = true;
                toCloseIdx.push_back(idx);
                return;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            toCloseIdx.push_back(idx);
            return;
        }
    }

    if (cur.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        toCloseIdx.push_back(idx);
    }
}


