#ifndef CMD_NICK_HPP
#define CMD_NICK_HPP
#include "Command.hpp"

class CmdNick : public Command {
public:
    virtual bool allowWhenUnregistered() const { return true; }
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
