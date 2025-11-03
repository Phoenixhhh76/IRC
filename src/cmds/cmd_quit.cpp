#include "cmds/cmd_quit.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: QUIT [reason]
// Disconnects the client from the server with an optional message.
void CmdQuit::execute(Server& srv, Client& cl, const IrcMessage& m) {
    std::string reason = m.params.empty() ? "Quit" : m.params[0];

    // Broadcast QUIT message to all related channel members
    const std::set<std::string>& channels = cl.channels();
    std::set<std::string> channelsCopy = channels; // Create copy because removeClientFromChannel will modify
    for (std::set<std::string>::const_iterator it = channelsCopy.begin();
         it != channelsCopy.end(); ++it) {
        // Notify other channel members that this user is leaving
        srv.broadcastToChannel(*it, cl.getFullPrefix() + " QUIT :" + reason, -1);

        // Remove client from channel
        srv.removeClientFromChannel(*it, cl.fd());
    }

    // Close connection (handled by Server::run(), just mark as needing close here)
    cl.closeNow();
}
