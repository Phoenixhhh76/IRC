#ifndef CMD_DCC_HPP
#define CMD_DCC_HPP
#include "Command.hpp"

class CmdDcc : public Command {
public:
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif

