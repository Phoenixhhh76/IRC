#include "Listener.hpp"
#include <cstring>      // std::memset, std::strerror
#include <string>
#include <cerrno>
#include <unistd.h>     // close
#include <fcntl.h>      // fcntl
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in, htons
#include <arpa/inet.h>  // INADDR_ANY

// --- private helpers ---------------------------------------------------------

void Listener::setNonBlocking() {
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK): " + std::string(std::strerror(errno)));
}

void Listener::setReuseAddr() {
    int on = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR): " + std::string(std::strerror(errno)));
}


// --- public ------------------------------------------------------------------

Listener::Listener(int port) : _fd(-1), _port(port) {
    if (port <= 0 || port > 65535)
        throw std::runtime_error("invalid port number");

    // _fd = ::socket(PF_INET, SOCK_STREAM, 0); Use PF_INET is not flexibla enough
    // if (_fd < 0)
    //     throw std::runtime_error("socket: " + std::string(std::strerror(errno)));
    this->setFd(::socket(AF_INET, SOCK_STREAM, 0));
    try {
        setReuseAddr();

        struct sockaddr_in sin;
        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
        sin.sin_port = htons(static_cast<unsigned short>(port));

        if (::bind(_fd, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) < 0)
            throw std::runtime_error("bind: " + std::string(std::strerror(errno)));

        if (::listen(_fd, 128) < 0)
            throw std::runtime_error("listen: " + std::string(std::strerror(errno)));

        //setNonBlocking();
    } catch (...) {
        ::close(_fd);
        throw;
    }
}

Listener::~Listener() {
    if (_fd >= 0)
        ::close(_fd);
}

int Listener::getFd() const { return _fd; }
int Listener::getPort() const { return _port; }

void Listener::setFd(int fd) {
    if (fd < 0)
        throw std::runtime_error("socket(): " + std::string(std::strerror(errno)));

    // 使用暫存 fd 先完成所有設定，成功後再寫入 _fd
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        ::close(fd);  // 設定失敗就關掉 socket
        throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK): " + std::string(std::strerror(errno)));
    }

    _fd = fd; // 最後一步才賦值，避免拋例外時留住已賦值的 fd
}

