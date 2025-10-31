#include "cmds/cmd_mode.hpp"
#include "Server.hpp"
#include "Replies.hpp"
#include <sstream>
#include <cstdlib>

// Execute | Format: MODE <channel> [modes] [parameters]
// Sets or displays channel modes. Requires channel operator privileges.
// Supported modes: +i (invite-only), +t (topic), +k (key), +l (limit), +o (operator)
void CmdMode::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    if (m.params.size() < 1) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "MODE"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    const std::string& target = m.params[0];

    // 如果目標是頻道
    if (target[0] == '#') {
        handleChannelMode(srv, cl, m);
    } else {
        // User mode handling
        handleUserMode(srv, cl, m);
    }
}

void CmdMode::handleChannelMode(Server& srv, Client& cl, const IrcMessage& m) {
    const std::string& ch = m.params[0];

    // 檢查頻道是否存在
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 取得頻道
    Channel& chan = srv.getChannel(ch);

    // 如果沒有提供 mode → 查詢目前 modes
    if (m.params.size() == 1) {
        std::ostringstream oss;
        oss << "+";
        if (chan.isInviteOnly()) oss << "i";
        if (chan.isTopicRestricted()) oss << "t";
        if (!chan.key().empty()) oss << "k";
        if (chan.userLimit() > 0) oss << "l";

        // 按規範以 324 numeric 回覆查詢：":server 324 <nick> <#chan> +modes [args]"
        cl.sendLine(RPL_CHANNELMODEIS(srv.serverName(), cl.getNick(), ch, oss.str(), std::string()));
        // 同時發送 329 頻道創建時間，提升客戶端顯示完整度
        {
            std::ostringstream ts;
            ts << chan.createdAt();
            cl.sendLine(RPL_CREATIONTIME(srv.serverName(), cl.getNick(), ch, ts.str()));
        }
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // irssi 常會發 MODE #chan b/e/I 來查詢列表（ban/except/invite-exempt）——本實作用空列表結束
    if (m.params.size() >= 2 && m.params[1] == "b") {
        cl.sendLine(RPL_ENDOFBANLIST(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    if (m.params.size() >= 2 && m.params[1] == "e") {
        cl.sendLine(RPL_ENDOFEXCEPTLIST(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    if (m.params.size() >= 2 && m.params[1] == "I") {
        cl.sendLine(RPL_ENDOFINVITELIST(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    processChannelMode(srv, cl, m, chan);
}

void CmdMode::processChannelMode(Server& srv, Client& cl, const IrcMessage& m, Channel& chan) {
    const std::string& ch = m.params[0];
    const std::string& modes = m.params[1];
    if (modes.empty()) return; // 無效，忽略
    bool add = (modes[0] == '+');
    if (modes.size() < 2) {
        // e.g. 單個字元如 "b" 的情況在上層已處理；其餘短字串不支援，忽略
        return;
    }
    char mode = modes[1];

    // 檢查用戶是否為 channel operator（只允許 operator 設定 modes）
    if (!srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 應用模式變更
    applyMode(srv, chan, mode, add, m);

    // 廣播 MODE 變化，使用完整 user 前綴，避免缺 host
    std::string args = (m.params.size() > 2 ? m.params[2] : "");
    srv.broadcastToChannel(ch, cl.getFullPrefix() + " MODE " + ch + " " + modes + (args.empty() ? "" : (" " + args)), -1);
}

void CmdMode::applyMode(Server& srv, Channel& chan, char mode, bool add, const IrcMessage& m) {
    int targetFd = -1;
    switch (mode) {
        case 'i': // Invite-only
            std::cout << "[DEBUG] Setting channel mode +i (invite-only): " << (add ? "ON" : "OFF") << std::endl;
            chan.setInviteOnly(add);
            std::cout << "[DEBUG] Channel is now " << (chan.isInviteOnly() ? "invite-only" : "not invite-only") << std::endl;
            break;
        case 't': // Topic restricted
            chan.setTopicRestricted(add);
            break;
        case 'k': // Channel key
            if (add && m.params.size() >= 3) {
                chan.setKey(m.params[2]);
            } else if (!add) {
                chan.setKey("");
            }
            break;
        case 'l': // User limit
            if (add && m.params.size() >= 3) {
                size_t limit = static_cast<size_t>(atoi(m.params[2].c_str()));
                chan.setUserLimit(limit);
            } else if (!add) {
                chan.setUserLimit(0);
            }
            break;
        case 'o': // Operator
            if (m.params.size() < 3) {
                break;  // Need target nick for operator mode
            }
            targetFd = srv.getFdByNick(m.params[2]);
            if (targetFd == -1 || !chan.hasClient(targetFd)) {
                break;  // Target user not found or not in channel
            }
            if (add) {
                chan.addOperator(targetFd);
            } else {
                chan.removeOperator(targetFd);
            }
            break;
        default:
            // 未知 mode，忽略
            break;
    }
}

void CmdMode::handleUserMode(Server& srv, Client& cl, const IrcMessage& m) {
    const std::string& target = m.params[0];

    // Check if user is trying to change their own modes
    if (target != cl.getNick()) {
        cl.sendLine("502 " + cl.getNick() + " :Cannot change mode for other users");
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // If no modes specified, show current modes
    if (m.params.size() < 2) {
        cl.sendLine("221 " + cl.getNick() + " +i");
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // For now, just acknowledge the mode change (irssi expects a response)
    const std::string& modes = m.params[1];
    cl.sendLine(cl.getFullPrefix() + " MODE " + cl.getNick() + " " + modes);
    srv.enableWriteForFd(cl.fd());
}
