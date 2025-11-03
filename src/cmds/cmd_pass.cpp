#include "cmds/cmd_pass.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: PASS <password>
// Sets the connection password. Must be called before NICK/USER registration.
void CmdPass::execute(Server& srv, Client& cl, const IrcMessage& m) {
    // Check if NICK or USER command has already been used
    if (cl.hasNick() || cl.hasUser()) {
        cl.sendLine(ERR_ALREADYREGISTRED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Check if already registered
    if (cl.registered()) {
        cl.sendLine(ERR_ALREADYREGISTRED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    if (m.params.empty()) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "PASS"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Check if password matches server setting
    if (!srv.checkPassword(m.params[0])) {
        cl.sendLine(ERR_PASSWDMISMATCH(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    cl.setPassOk();
    if (cl.tryFinishRegister())
        srv.sendWelcome(cl);
}
