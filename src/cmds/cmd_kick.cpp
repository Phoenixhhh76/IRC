#include "cmds/cmd_kick.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: KICK <channel> <nick> [reason]
// Kicks a user from a channel. Requires channel operator privileges.
void CmdKick::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // KICK <channel> <user> [reason]
    if (m.params.size() < 2) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "KICK"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    const std::string& ch = m.params[0];
    const std::string& target = m.params[1];
    std::string reason = m.params.size() > 2 ? m.params[2] : cl.getNick();

    // 檢查頻道是否存在
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查發起者是否在頻道
    if (!srv.isChannelMember(ch, cl.fd())) {
    cl.sendLine(ERR_NOTONCHANNEL(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查是否為 channel operator
    if (!srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 找目標用戶（需要 Server 提供 getter）
    int targetFd = srv.getFdByNick(target);
    if (targetFd == -1) {
        cl.sendLine(ERR_NOSUCHNICK(srv.serverName(), target));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查目標是否在頻道
    if (!srv.isChannelMember(ch, targetFd)) {
        cl.sendLine(ERR_USERNOTINCHANNEL(srv.serverName(), target, ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 執行 KICK：從頻道移除並廣播（使用完整 user 前綴）
    srv.broadcastToChannel(ch, cl.getFullPrefix() + " KICK " + ch + " " + target + " :" + reason, -1);
    srv.removeClientFromChannel(ch, targetFd);
}
