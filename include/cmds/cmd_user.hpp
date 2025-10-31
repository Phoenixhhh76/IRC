#ifndef CMD_USER_HPP
#define CMD_USER_HPP
#include "Command.hpp"

class CmdUser : public Command {
public:
    virtual bool allowWhenUnregistered() const { return true; }
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
