#include "cmds/cmd_cap.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: CAP LS / CAP LIST / CAP REQ / CAP END
// Client capability negotiation. We'll just ignore it for simplicity.
void CmdCap::execute(Server& srv, Client& cl, const IrcMessage& m) {
    (void)srv;
    (void)cl;
    (void)m;
    
    // CAP 協商：irssi 會發送 CAP LS
    // 我們簡單地忽略它（不需要響應）
    // 註冊後的命令會正常處理
    
    // 如果需要響應：
    // if (m.params.size() > 0 && m.params[0] == "LS") {
    //     cl.sendLine("CAP * LS :");
    //     srv.enableWriteForFd(cl.fd());
    // }
}

