#include "cmds/cmd_privmsg.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: PRIVMSG <target> :<message>
// Sends a message to a channel or user.
void CmdPrivmsg::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered())
        return;

    if (m.params.size() < 1) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "PRIVMSG"));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    const std::string target = m.params[0];
    const std::string text = (m.params.size() >= 2) ? m.params.back() : "";
    if (text.empty()) {
        cl.sendLine(ERR_NOTEXTTOSEND(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查是否是發給 Bot 的訊息（不檢查 Bot 是否存在，因為它是內建的）
    if (target == "ft_irc_Bot") {
        srv.handleBotMessage(cl.getNick(), cl.getNick(), text);
        // 不需要檢查用戶是否存在，直接返回
        cl.sendLine(RPL_NOTICE(srv.serverName(), cl.getNick(), "Bot message sent"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 先提供可編譯的回饋
    cl.sendLine(RPL_NOTICE(srv.serverName(), cl.getNick(), "PRIVMSG queued")); // 暫時回一條提示
    srv.enableWriteForFd(cl.fd());

    if (target.size() > 0 && target[0] != '#') {
        if (!srv.isNickInUse(target)) {
            cl.sendLine(ERR_NOSUCHNICK(srv.serverName(), target));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        srv.sendToNick(target, cl.getFullPrefix() + " PRIVMSG " + target + " :" + text);
        return;
    }

    if (!srv.channelExists(target) || !srv.isChannelMember(target, cl.fd())) {
        cl.sendLine(ERR_CANNOTSENDTOCHAN(srv.serverName(), cl.getNick(), target));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    srv.broadcastToChannel(target, cl.getFullPrefix() + " PRIVMSG " + target + " :" + text, cl.fd());

    // 檢查頻道訊息中是否包含 Bot 命令或關鍵字觸發
    if (text == "!hello" || text == "!help" || text == "!time" || text == "!stats" || text == "!ping" || text == "!info"
        || (text.size() > 5 && text.substr(0,5) == "!ask ")
        || text.find("42") != std::string::npos) {
        srv.handleBotMessage(cl.getNick(), target, text);
    } else {
        // 大小寫不敏感檢查 irc
        std::string lower = text;
        for (size_t i = 0; i < lower.size(); ++i) lower[i] = std::tolower(lower[i]);
        if (lower.find("irc") != std::string::npos) {
            srv.handleBotMessage(cl.getNick(), target, text);
        }
    }
}
