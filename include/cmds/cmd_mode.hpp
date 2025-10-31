#ifndef CMD_MODE_HPP
#define CMD_MODE_HPP

#include "Command.hpp"

class CmdMode : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
private:
    void handleChannelMode(Server& srv, Client& cl, const IrcMessage& m);
    void handleUserMode(Server& srv, Client& cl, const IrcMessage& m);
    void processChannelMode(Server& srv, Client& cl, const IrcMessage& m, Channel& chan);
    void applyMode(Server& srv, Client& cl, Channel& chan, char mode, bool add, const IrcMessage& m);
};

#endif
