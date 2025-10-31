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
    std::string _inbuf;    // 收到但尚未切出完整行的資料
    std::string _outbuf;   // 待傳送資料（可能包含多行）
    std::string _nick;
    std::string _user;
    std::string _realname;
    std::string _host;
    std::set<std::string> _channels;   // client 所屬的頻道名稱

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
    void closeNow();// 明確關閉 socket（將 _fd 設為 -1，避免二次關閉 broken pipe）

    // 從 socket 讀資料到 _inbuf；回傳：true=連線仍在，false=應關閉（對端關閉或讀錯）
    bool readFromSocket();

    // 切出一行（不含結尾的 CRLF）。有行則回 true 並放入 out；沒有完整行回 false
    bool popLine(std::string& out);

    // 把 _outbuf 的內容盡量送出；錯誤（非 EAGAIN/EWOULDBLOCK）則丟例外或交由外層處理
    void flushOutbuf();

    // 是否還有待發資料
    bool isOutbufEmpty() const;

    // 佇列一行待發資料（自動補上 \r\n）
    void sendLine(const std::string& line);

    // 也可直接佇列原始位元組（不自動加 CRLF）
    void appendRaw(const std::string& data);

    void setPassOk();
    const std::string getNick() const;
    void setNick(const std::string& nickname);
    const std::string getUser() const;
    void setUser(const std::string& username, const std::string& real);
    const std::string& getHost() const;
    void setHost(const std::string& h);
    const std::string& getRealname() const;
    std::string getFullPrefix() const;  // 返回 :nick!user@host 格式


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
