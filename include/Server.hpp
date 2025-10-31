#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <string>
#include <fstream>
#include "bonus/Dcc.hpp"

#include "Listener.hpp"
#include "Client.hpp"
#include "acceptClients.hpp"
#include "Parser.hpp"
#include "Channel.hpp"
#include "CommandRegistry.hpp"
#include "utils.hpp"

class Server {
private:
    Listener            _listener;
    std::vector<pollfd> _pfds;
    std::map<int, Client*> _clients; // fd → pointer to Client
    std::map<std::string, int>  _nick2fd;    // nick -> fd
    std::string                 _servername;
    std::string                 _password;    // 伺服器密碼
    std::map<std::string, Channel> _channels;
    CommandRegistry _cmds;
    std::set<std::string> _operators; // operator 暱稱集合
    std::string _botNick; // Bot 的暱稱
    int _botFd; // Bot 的檔案描述符（虛擬）

    std::map<int, DccSession> _dccByFd; // dccSockFd -> session

    // put in private instead of public to protect from copy
    Server(const Server&);
    Server& operator=(const Server&);

public:

// need to clean to .cpp file
    explicit Server(int port, const std::string& password);
    ~Server();

    void run();
    void initCommands();
    void handleIrcMessage(Client& cl, const IrcMessage& m);
    const std::string& serverName() const;

    void broadcastToChannel(const std::string& ch, const std::string& line, int except_fd);
    // add for IRC message
    void enableWriteForFd(int fd);               // 找到對應 pollfd，開啟 POLLOUT
    bool isNickInUse(const std::string& nick) const;
    void takeNick(Client& cl, const std::string& newNick); // 更新 client 與 _nick2fd
    void sendWelcome(Client& cl);                // 發 001

    bool addClientToChannel(const std::string& ch, int fd);
    bool removeClientFromChannel(const std::string& ch, int fd);
    bool isChannelMember(const std::string& ch, int fd) const;
    bool channelExists(const std::string& ch) const;

    void sendToNick(const std::string& nick, const std::string& line);

    // --- 密碼與操作員相關 ---
    bool checkPassword(const std::string& pwd) const;
    bool isOperator(const std::string& nick) const;
    void addOperator(const std::string& nick);

    // --- 便利 getter（供指令使用）---
    int getFdByNick(const std::string& nick) const;
    Channel& getChannel(const std::string& ch);
    
    Client*       getClientByFd(int fd);
    const Client* getClientByFd(int fd) const;

    // --- 供指令檢查 channel operator ---
    bool isChannelOperator(const std::string& ch, int fd) const;

    // --- Bot 功能 ---
    void initBot();
    void handleBotMessage(const std::string& senderNick, const std::string& target, const std::string& message);
    void botSendMessage(const std::string& target, const std::string& message);
    void botSendNotice(const std::string& nick, const std::string& message);

    // --- DCC 功能 ---
    void handleDccSend(int fd, const IrcMessage& m);
    void processDccPollEvent(size_t idx, std::vector<size_t>& toCloseIdx);

    // --- run() 輔助函數 ---
    void processPollEvent(size_t idx, std::vector<int>& toAddFds, std::vector<size_t>& toCloseIdx);
    void closeClient(size_t idx);
    void addNewClient(int fd);
};

#endif

