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
    // More than one parameter (e.g. "NICK a b") → 432
    if (m.params.size() > 1) {
        cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), m.params[0]));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    const std::string& nick = m.params[0];
    // Validity check → 432
    if (!isValidNick(nick)) {
        cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), nick));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Registered users renaming: allowed, but check duplicates; unregistered also check duplicates
    if (srv.isNickInUse(nick) && (!cl.hasNick() || nick != cl.getNick())) {
        cl.sendLine(ERR_NICKNAMEINUSE(srv.serverName(), nick));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    // Save old prefix for broadcasting
    std::string oldPrefix = cl.getFullPrefix();

    // Update nickname
    srv.takeNick(cl, nick);

    // Broadcast nickname change to all related channels
    const std::set<std::string>& channels = cl.channels();
    for (std::set<std::string>::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        srv.broadcastToChannel(*it, oldPrefix + " NICK :" + nick, -1);
    }

    // If this is part of registration process, send welcome message
    if (cl.tryFinishRegister())
        srv.sendWelcome(cl);
}
