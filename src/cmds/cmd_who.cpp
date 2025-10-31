#include "cmds/cmd_who.hpp"
#include "Server.hpp"
#include "Replies.hpp"
#include <sstream>

// Execute | Format: WHO [<name>|<mask>|#channel]
// Minimal implementation to satisfy irssi: supports WHO #channel and WHO <nick>.
void CmdWho::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    std::string name = (m.params.size() >= 1) ? m.params[0] : std::string("*");

    // WHO #channel → list members with user/host
    if (!name.empty() && name[0] == '#') {
        if (!srv.channelExists(name)) {
            // Channel not found; still terminate WHO list
            cl.sendLine(RPL_ENDOFWHO(srv.serverName(), cl.getNick(), name));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        Channel& chan = srv.getChannel(name);
        const std::set<int>& mem = chan.members();
        for (std::set<int>::const_iterator it = mem.begin(); it != mem.end(); ++it) {
            const Client* member = srv.getClientByFd(*it);
            if (!member) continue;
            std::string user = member->getUser().empty() ? std::string("unknown") : member->getUser();
            std::string host = member->getHost().empty() ? std::string("unknown") : member->getHost();
            std::string flags = "H"; // Assume here (not away)
            if (chan.isOperator(*it)) flags += "@"; // show op
            std::string hops = "0";
            std::string real = member->getRealname().empty() ? member->getNick() : member->getRealname();
            cl.sendLine(RPL_WHOREPLY(srv.serverName(), cl.getNick(), name, user, host, srv.serverName(), member->getNick(), flags, hops, real));
            srv.enableWriteForFd(cl.fd());
        }
        cl.sendLine(RPL_ENDOFWHO(srv.serverName(), cl.getNick(), name));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // WHO <nick> → single user (best-effort minimal)
    int fd = srv.getFdByNick(name);
    if (fd != -1) {
        const Client* member = srv.getClientByFd(fd);
        if (member) {
            std::string user = member->getUser().empty() ? std::string("unknown") : member->getUser();
            std::string host = member->getHost().empty() ? std::string("unknown") : member->getHost();
            std::string flags = "H";
            std::string hops = "0";
            std::string real = member->getRealname().empty() ? member->getNick() : member->getRealname();
            cl.sendLine(RPL_WHOREPLY(srv.serverName(), cl.getNick(), std::string("*"), user, host, srv.serverName(), member->getNick(), flags, hops, real));
            srv.enableWriteForFd(cl.fd());
        }
    }
    cl.sendLine(RPL_ENDOFWHO(srv.serverName(), cl.getNick(), name));
    srv.enableWriteForFd(cl.fd());
}
