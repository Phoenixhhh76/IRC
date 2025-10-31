#ifndef BONUS_DCC_HPP
#define BONUS_DCC_HPP

#include <string>
#include <fstream>

struct DccSession {
    int             sockFd;
    int             ownerFd;
    std::string     filename;
    std::string     savePath;
    std::ofstream*  ofs;
    bool            connecting;
    bool            finished;

    DccSession();
    DccSession(const DccSession& other);
    DccSession& operator=(const DccSession& other);
    ~DccSession();
};

#endif

