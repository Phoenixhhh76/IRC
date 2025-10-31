#include "cmds/cmd_part.hpp"
#include "Server.hpp"
#include "Replies.hpp"
#include <vector>
#include <string>

// Execute | Format: PART <channel> [reason]
// Leaves the specified channel.
void CmdPart::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered())
        return;

    if (m.params.size() < 1) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "PART"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // local splitter for comma-separated channel list
    std::vector<std::string> chans;
    {
        const std::string& list = m.params[0];
        std::string cur;
        for (size_t i = 0; i < list.size(); ++i) {
            char c = list[i];
            if (c == ',') { chans.push_back(cur); cur.clear(); }
            else { cur.push_back(c); }
        }
        chans.push_back(cur);
    }

    // Extract optional reason from params[1]
    std::string reason = (m.params.size() > 1) ? m.params[1] : "";
    const std::string prefix = cl.getFullPrefix();
    
    for (size_t i = 0; i < chans.size(); ++i) {
        const std::string& ch = chans[i];

        if (!cl.inChannel(ch)) {
            cl.sendLine(ERR_NOTONCHANNEL(srv.serverName(), cl.getNick(), ch));
            srv.enableWriteForFd(cl.fd());
            continue;
        }

        // Remove from client's channel set first
        cl.removeChannel(ch);
        
        std::string partMsg = prefix + " PART " + ch;
        if (!reason.empty()) {
            partMsg += " :" + reason;
        }
        
        // Echo PART to self with full user prefix
        cl.sendLine(partMsg);
        srv.enableWriteForFd(cl.fd());

        // Then remove from server channel and broadcast to others
        if (srv.isChannelMember(ch, cl.fd())) {
            srv.removeClientFromChannel(ch, cl.fd());
            srv.broadcastToChannel(ch, partMsg, -1);
        }
    }
}
