#include "cmds/cmd_ping.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: PING <token>
// Server sends PING to check if client is still connected. Client responds with PONG.
void CmdPing::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!m.params.empty()) {
        cl.sendLine(RPL_PING(srv.serverName(), m.params.back())); // Compose PONG
        srv.enableWriteForFd(cl.fd());
    }
}
