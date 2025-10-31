#ifndef CMD_CAP_HPP
#define CMD_CAP_HPP

#include "../Command.hpp"
#include "../Server.hpp"
#include "../Client.hpp"
#include "../Parser.hpp"

// CAP command - Client Capability Negotiation
// Ignored for simplicity
class CmdCap : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif // CMD_CAP_HPP


