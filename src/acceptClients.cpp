#include "acceptClients.hpp"
#include <vector>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>      // NI_MAXHOST, NI_NUMERICHOST
#include <arpa/inet.h>  // inet_ntop

static void set_nonblock_fd(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        ::close(fd);  // 設定失敗就關掉 socket
        throw std::runtime_error("fcntl(F_SETFL, O_NONBLOCK): " + std::string(std::strerror(errno)));
    }
}

// Return a vector of pairs containing new connection file descriptors and their IP addresses
std::vector<std::pair<int, std::string> > acceptClients(int listen_fd) {
    std::vector<std::pair<int, std::string> > connections;
    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int cs = ::accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (cs < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // No more connections
            if (errno == EINTR) continue; // Retry if interrupted
            break;
        }
        try {
            set_nonblock_fd(cs);  // Will close socket on failure

            // 获取主机名，首选使用数字形式的IP地址
            char hostbuf[NI_MAXHOST];
            int rc = getnameinfo((struct sockaddr*)&client_addr, addr_len,
                                hostbuf, sizeof(hostbuf),
                                NULL, 0, NI_NUMERICHOST);

            std::string host;
            if (rc == 0) {
                host = hostbuf;
            } else {
                // 回退到手动IP转换
                char ipstr[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(client_addr.sin_addr), ipstr, sizeof(ipstr))) {
                    host = ipstr;
                } else {
                    host = "unknown";
                }
            }
            connections.push_back(std::make_pair(cs, host));
        } catch (...) {
            throw;  // Error already handled in set_nonblock_fd, rethrow
        }
    }
    return connections;
}
