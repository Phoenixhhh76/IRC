// prototype come from srv_create

#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <stdexcept>

/**
 * @brief Helper class to create and manage a non-blocking TCP listening socket.
 *
 * Usage example:
 *   Listener listener(port);
 *   int fd = listener.getFd();
 *   // then add fd to poll/select
 */
class Listener {

public:
    explicit Listener(int port);
    ~Listener();

    void setFd(int fd);
    int  getFd() const;
    int  getPort() const;

private:
    int _fd;
    int _port;

    void setNonBlocking();
    void setReuseAddr();

};

#endif // LISTENER_HPP
