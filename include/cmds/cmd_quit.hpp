#ifndef CMD_QUIT_HPP
#define CMD_QUIT_HPP

#include "Command.hpp"

// Execute | Format: QUIT [reason]
// Disconnects the client from the server with an optional message.
class CmdQuit : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m);
};

#endif
