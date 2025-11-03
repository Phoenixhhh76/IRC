#include "cmds/cmd_cap.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: CAP LS / CAP LIST / CAP REQ / CAP END
// Client capability negotiation. We'll just ignore it for simplicity.
void CmdCap::execute(Server& srv, Client& cl, const IrcMessage& m) {
    (void)srv;
    (void)cl;
    (void)m;
    
    // CAP negotiation: irssi will send CAP LS
    // We simply ignore it (no response needed)
    // Commands after registration will be processed normally
    
    // If response needed:
    // if (m.params.size() > 0 && m.params[0] == "LS") {
    //     cl.sendLine("CAP * LS :");
    //     srv.enableWriteForFd(cl.fd());
    // }
}

