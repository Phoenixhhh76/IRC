#ifndef COMMAND_HPP
#define COMMAND_HPP
#include "Parser.hpp"
#include "Client.hpp"
class Server; 

class Command {
public:
    virtual ~Command() {}
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m) = 0;
    // option
    virtual bool allowWhenUnregistered() const { return false; }
};
#endif