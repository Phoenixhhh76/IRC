#ifndef CMD_PART_HPP
#define CMD_PART_HPP
#include "Command.hpp"

class CmdPart : public Command {
public:
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
