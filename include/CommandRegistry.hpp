#ifndef COMMANDREGISTRY_HPP
#define COMMANDREGISTRY_HPP
#include <map>
#include <string>
#include "Command.hpp"

class CommandRegistry {
private:
    std::map<std::string, Command*> _tab; // "NICK" → new CmdNick
public:
    ~CommandRegistry(); // loop delete
    void registerCmd(const std::string& upName, Command* cmd) { _tab[upName] = cmd; }
    Command* find(const std::string& upName) const;
    bool dispatch(const std::string& upName, Server& srv, Client& cl, const IrcMessage& m) const;
};
#endif