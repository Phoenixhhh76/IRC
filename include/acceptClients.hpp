#ifndef ACCEPTCLIENTS_HPP
#define ACCEPTCLIENTS_HPP
#include <vector>
#include <string>
#include <utility>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<std::pair<int, std::string> > acceptClients(int listen_fd);
#endif