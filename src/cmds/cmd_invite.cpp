#include "cmds/cmd_invite.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: INVITE <nick> <channel>
// Invites a user to join an invite-only channel. Requires channel operator if channel is +i.
void CmdInvite::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // INVITE <nickname> <channel>
    if (m.params.size() < 2) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "INVITE"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    const std::string& target = m.params[0];
    const std::string& ch = m.params[1];

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

    // 若頻道是 invite-only，檢查是否為 channel operator
    Channel& chan = srv.getChannel(ch);
    if (chan.isInviteOnly() && !srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 找目標用戶
    int targetFd = srv.getFdByNick(target);
    if (targetFd == -1) {
        cl.sendLine(ERR_NOSUCHNICK(srv.serverName(), target));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查目標是否已在頻道
    if (srv.isChannelMember(ch, targetFd)) {
        cl.sendLine(ERR_USERONCHANNEL(srv.serverName(), target, ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 發送邀請通知給目標,完整前綴 :nick!user@host
    const std::string prefix = cl.getFullPrefix();
    std::string inviteMsg = prefix + " INVITE " + target + " :" + ch;
    srv.sendToNick(target, inviteMsg);

    // 回覆給發起者（格式：":server 341 nick target #channel"）
    cl.sendLine(":" + srv.serverName() + " 341 " + cl.getNick() + " " + target + " " + ch);
    srv.enableWriteForFd(cl.fd());

    // 標記該用戶為已邀請
    std::cout << "[DEBUG] Inviting user: " << target << std::endl;
    std::cout << "[DEBUG] Target nickname (lowercase): " << toLower(target) << std::endl;
    chan.inviteNick(target);  // 以 nick 存
    std::cout << "[DEBUG] Invitation stored" << std::endl;

}
