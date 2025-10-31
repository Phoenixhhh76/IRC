#ifndef CMD_TOPIC_HPP
#define CMD_TOPIC_HPP

#include "Command.hpp"

class CmdTopic : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
