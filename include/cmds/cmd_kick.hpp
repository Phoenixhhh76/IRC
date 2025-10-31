#ifndef CMD_KICK_HPP
#define CMD_KICK_HPP

#include "Command.hpp"

class CmdKick : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
