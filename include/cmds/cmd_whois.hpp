#ifndef CMD_WHOIS_HPP
#define CMD_WHOIS_HPP

#include "../Command.hpp"
#include "../Server.hpp"
#include "../Client.hpp"
#include "../Parser.hpp"

// WHOIS command - Returns information about a user
class CmdWhois : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif // CMD_WHOIS_HPP

