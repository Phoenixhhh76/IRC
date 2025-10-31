#ifndef CMD_WHO_HPP
#define CMD_WHO_HPP

#include "Command.hpp"

class CmdWho : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
