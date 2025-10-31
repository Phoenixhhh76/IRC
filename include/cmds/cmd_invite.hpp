#ifndef CMD_INVITE_HPP
#define CMD_INVITE_HPP

#include "Command.hpp"

class CmdInvite : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
