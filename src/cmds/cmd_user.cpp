#include "cmds/cmd_user.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: USER <username> <mode> <unused> :<realname>
// Sets the client's username and realname during registration.
void CmdUser::execute(Server& srv, Client& cl, const IrcMessage& m) {
    std::cout << "[DEBUG] CmdUser::execute called from fd=" << cl.fd()
              << ", params.size()=" << m.params.size() << std::endl;
    for (size_t i = 0; i < m.params.size(); ++i) {
        std::cout << "[DEBUG]   params[" << i << "]='" << m.params[i] << "'" << std::endl;
    }

    if (cl.registered()) {
        cl.sendLine(ERR_ALREADYREGISTRED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    if (m.params.size() < 4) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "USER"));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    const std::string username = m.params[0];
    const std::string realname = m.params[3];
    const std::string hostname = m.params[2];  // Use the hostname parameter from USER command
    cl.setUser(username, realname);
    if (hostname != "*")  // Only set if it's not the placeholder
        cl.setHost(hostname);
    if (cl.tryFinishRegister())
        srv.sendWelcome(cl);
}
