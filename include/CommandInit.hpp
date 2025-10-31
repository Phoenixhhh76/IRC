#ifndef COMMAND_INIT_HPP
#define COMMAND_INIT_HPP

#include "CommandRegistry.hpp"
class Server;

void registerAllCommands(CommandRegistry& registry, Server& srv);

#endif
