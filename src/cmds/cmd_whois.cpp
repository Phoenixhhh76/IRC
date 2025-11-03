#include "cmds/cmd_whois.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: WHOIS <nickname>
// Returns information about a user.
void CmdWhois::execute(Server& srv, Client& cl, const IrcMessage& m) {
    (void)srv;
    (void)cl;
    (void)m;
    
    // WHOIS command temporarily simply ignored
    // irssi will automatically send WHOIS on connect, we can simply respond
}

