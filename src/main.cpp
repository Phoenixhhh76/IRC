#include "Server.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstdlib>   // atoi

int main(int argc, char** argv) {
    // 安裝訊號處理，支援優雅關機（SIGINT/SIGTERM）並忽略 SIGPIPE
    installSignalHandlers();

    // 根據規格：./ircserv <port> <password>
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    try {
        Server server(port, password);
        std::cout << "IRC server starting on port " << port << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
