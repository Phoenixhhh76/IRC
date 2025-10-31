#include "CommandRegistry.hpp"
CommandRegistry::~CommandRegistry() {
    for (std::map<std::string, Command*>::iterator it = _tab.begin(); it != _tab.end(); ++it)
        delete it->second;
}
Command* CommandRegistry::find(const std::string& upName) const {
    std::map<std::string, Command*>::const_iterator it = _tab.find(upName);
    return it == _tab.end() ? 0 : it->second;
}
bool CommandRegistry::dispatch(const std::string& upName, Server& srv, Client& cl, const IrcMessage& m) const {
    Command* c = find(upName);
    if (!c) return false;
    c->execute(srv, cl, m);
    return true;
}
