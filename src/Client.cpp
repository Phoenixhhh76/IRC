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
    // Safe cleanup in destructor; if closeNow() was already called, _fd will be -1, won't close again
    closeNow();
}

// Copy constructor and copy assignment are intentionally not implemented.
// Client is non-copyable in this C++98 codebase because it owns a socket fd.

void Client::closeNow() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1; // Avoid double close
    }
}
int Client::fd() const { return _fd; }

bool Client::readFromSocket() {
    char buf[4096];
    for (;;) {
        ssize_t n = ::recv(_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            //std::cout << "[IRC] Client fd=" << _fd << " recv " << n << " bytes" << std::endl;
            _inbuf.append(buf, buf + n);
            // Continue reading until EAGAIN (safe for edge/level triggered)
            continue;
        }
        if (n == 0) {
            // Peer closed
            //std::cout << "[IRC] Client fd=" << _fd << " (" << _nick << ") recv EOF, closing" << std::endl;
            return false;
        }
        // n < 0
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // This round of reading complete
            //std::cout << "[IRC] Client fd=" << _fd << " recv EAGAIN/EWOULDBLOCK" << std::endl;
            return true;
        }
        if (errno == EINTR) {
            // Interrupted by signal, retry
            //std::cout << "[IRC] Client fd=" << _fd << " recv EINTR, retrying" << std::endl;
            continue;
        }
        // Other errors: return false for upper layer cleanup (or could throw exception)
        //std::cout << "[IRC] Client fd=" << _fd << " recv error: " << std::strerror(errno) << std::endl;
        return false;
    }
}

bool Client::popLine(std::string& out) {
    // IRC officially ends with "\r\n"; tolerate tools with only '\n' (e.g. some tests)
    std::string::size_type pos = _inbuf.find("\r\n");
    if (pos != std::string::npos) {
        out.assign(_inbuf, 0, pos);
        _inbuf.erase(0, pos + 2);
        return true;
    }
    // Lenient: only '\n'
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
            // Continue trying to send more; until can't send
            continue;
        }
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Currently can't write: enable POLLOUT via poll, wait for next round
                return;
            }
            if (errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            }
            // Other errors: let outer layer handle (could also change to return false)
            throw std::runtime_error("send: " + std::string(std::strerror(errno)));
        }
        // n == 0 almost never happens (for TCP send), break just in case
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

// Just check if Nick and User is done
bool Client::tryFinishRegister() {
    // Ensure password is set before NICK and USER
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