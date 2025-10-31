#include "cmds/cmd_quit.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: QUIT [reason]
// Disconnects the client from the server with an optional message.
void CmdQuit::execute(Server& srv, Client& cl, const IrcMessage& m) {
    std::string reason = m.params.empty() ? "Quit" : m.params[0];

    // 廣播 QUIT 訊息給所有相關頻道的成員
    const std::set<std::string>& channels = cl.channels();
    std::set<std::string> channelsCopy = channels; // 創建副本，因為 removeClientFromChannel 會修改
    for (std::set<std::string>::const_iterator it = channelsCopy.begin();
         it != channelsCopy.end(); ++it) {
        // 通知頻道其他成員這個用戶離開了
        srv.broadcastToChannel(*it, cl.getFullPrefix() + " QUIT :" + reason, -1);

        // 從頻道移除客戶端
        srv.removeClientFromChannel(*it, cl.fd());
    }

    // 關閉連線（由 Server::run() 處理，這裡只是標記需要關閉）
    cl.closeNow();
}
