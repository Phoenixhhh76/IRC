#ifndef CMD_PING_HPP
#define CMD_PING_HPP
#include "Command.hpp"

class CmdPing : public Command {
public:
    virtual bool allowWhenUnregistered() const { return true; }
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
