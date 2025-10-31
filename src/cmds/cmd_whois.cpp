#include "cmds/cmd_whois.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: WHOIS <nickname>
// Returns information about a user.
void CmdWhois::execute(Server& srv, Client& cl, const IrcMessage& m) {
    (void)srv;
    (void)cl;
    (void)m;
    
    // WHOIS 命令暫時簡單忽略
    // irssi 會在連接時自動發送 WHOIS，我們簡單回應即可
}

