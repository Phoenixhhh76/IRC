#include "cmds/cmd_dcc.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: DCC SEND <filename> <IP> <port>
// Handles DCC file transfer requests
void CmdDcc::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered())
        return;

    srv.handleDccSend(cl.fd(), m);
}

