#ifndef CMD_PRIVMSG_HPP
#define CMD_PRIVMSG_HPP
#include "Command.hpp"

class CmdPrivmsg : public Command {
public:
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
