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

    // Check if channel exists
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Check if initiator is in channel
    if (!srv.isChannelMember(ch, cl.fd())) {
    cl.sendLine(ERR_NOTONCHANNEL(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // If channel is invite-only, check if channel operator
    Channel& chan = srv.getChannel(ch);
    if (chan.isInviteOnly() && !srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Find target user
    int targetFd = srv.getFdByNick(target);
    if (targetFd == -1) {
        cl.sendLine(ERR_NOSUCHNICK(srv.serverName(), target));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Check if target is already in channel
    if (srv.isChannelMember(ch, targetFd)) {
        cl.sendLine(ERR_USERONCHANNEL(srv.serverName(), target, ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Send invite notification to target, full prefix :nick!user@host
    const std::string prefix = cl.getFullPrefix();
    std::string inviteMsg = prefix + " INVITE " + target + " :" + ch;
    srv.sendToNick(target, inviteMsg);

    // Reply to initiator (format: ":server 341 nick target #channel")
    cl.sendLine(":" + srv.serverName() + " 341 " + cl.getNick() + " " + target + " " + ch);
    srv.enableWriteForFd(cl.fd());

    // Mark user as invited
    //std::cout << "[IRC] Inviting user: " << target << std::endl;
    //std::cout << "[IRC] Target nickname (lowercase): " << toLower(target) << std::endl;
    chan.inviteNick(target);  // Store by nick
    //std::cout << "[IRC] Invitation stored" << std::endl;

}
