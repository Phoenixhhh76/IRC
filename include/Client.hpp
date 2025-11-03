#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include "Channel.hpp"

/**
 * @brief 表示一個連線中的用戶；負責 socket 讀寫緩衝與行切割。
 */
class Client {
private:
    int         _fd;
    std::string _inbuf;    // Received but not yet split into complete lines
    std::string _outbuf;   // Data to send (may contain multiple lines)
    std::string _nick;
    std::string _user;
    std::string _realname;
    std::string _host;
    std::set<std::string> _channels;   // Channel names this client belongs to

    bool _isAuthenticated;
    bool _hasPass;
    bool _hasNick;
    bool _hasUser;
    bool _registered;
    
private:
    // Non-copyable in C++98: copying a Client (owns a socket fd) is unsafe.
    Client(const Client&);
    Client& operator=(const Client&);

public:
    explicit Client(int fd);
    ~Client();

    int  fd() const;

    // Note: copy ctor / assignment intentionally NOT declared/implemented.
    // Client is non-copyable in this C++98 codebase because it owns a socket fd.
    void closeNow();// Explicitly close socket (set _fd to -1, avoid double close broken pipe)

    // Read data from socket to _inbuf; return: true=connection alive, false=should close (peer closed or read error)
    bool readFromSocket();

    // Extract one line (without trailing CRLF). Return true and put in out if has line; return false if no complete line
    bool popLine(std::string& out);

    // Send as much of _outbuf as possible; error (not EAGAIN/EWOULDBLOCK) throws exception or handled by upper layer
    void flushOutbuf();

    // Whether there's data to send
    bool isOutbufEmpty() const;

    // Queue one line to send (automatically append \r\n)
    void sendLine(const std::string& line);

    // Can also directly queue raw bytes (don't auto-add CRLF)
    void appendRaw(const std::string& data);

    void setPassOk();
    const std::string getNick() const;
    void setNick(const std::string& nickname);
    const std::string getUser() const;
    void setUser(const std::string& username, const std::string& real);
    const std::string& getHost() const;
    void setHost(const std::string& h);
    const std::string& getRealname() const;
    std::string getFullPrefix() const;  // Return :nick!user@host format


    bool hasNick() const;
    bool hasUser() const;
    bool hasPass() const; 
    bool registered() const;

    bool tryFinishRegister();

    bool isAuthenticated() const;
    void setAuthenticated(bool auth);

    // ---- for channel ----
    void addChannel(const std::string& ch);
    void removeChannel(const std::string& ch);
    bool inChannel(const std::string& ch) const;
    const std::set<std::string>& channels() const;

};

#endif // CLIENT_HPP
