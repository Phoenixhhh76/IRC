#include "cmds/cmd_nick.hpp"
#include "Server.hpp"
#include "Replies.hpp"
#include <cctype>
#include <string>

// Execute | Format: NICK <nickname>
// Sets or changes the client's nickname.
void CmdNick::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (m.params.empty()) {
        cl.sendLine(ERR_NONICKNAMEGIVEN(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    // 多於一個參數（例如 "NICK a b"）→ 432
    if (m.params.size() > 1) {
        cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), m.params[0]));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    const std::string& nick = m.params[0];
    // 合法性檢查 → 432
    if (!isValidNick(nick)) {
        cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), nick));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 已註冊者改名：允許，但要檢查重複；未註冊也要檢查重複
    if (srv.isNickInUse(nick) && (!cl.hasNick() || nick != cl.getNick())) {
        cl.sendLine(ERR_NICKNAMEINUSE(srv.serverName(), nick));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    // 保存旧的前缀用于广播
    std::string oldPrefix = cl.getFullPrefix();

    // 更新昵称
    srv.takeNick(cl, nick);

    // 广播昵称变更到所有相关频道
    const std::set<std::string>& channels = cl.channels();
    for (std::set<std::string>::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        srv.broadcastToChannel(*it, oldPrefix + " NICK :" + nick, -1);
    }

    // 如果这是注册过程的一部分，发送欢迎消息
    if (cl.tryFinishRegister())
        srv.sendWelcome(cl);
}
