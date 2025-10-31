#ifndef CMD_PASS_HPP
#define CMD_PASS_HPP
#include "Command.hpp"

class CmdPass : public Command {
public:
    virtual bool allowWhenUnregistered() const { return true; }
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif

