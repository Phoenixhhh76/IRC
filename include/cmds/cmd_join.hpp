#ifndef CMD_JOIN_HPP
#define CMD_JOIN_HPP
#include "Command.hpp"

class CmdJoin : public Command {
public:
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
