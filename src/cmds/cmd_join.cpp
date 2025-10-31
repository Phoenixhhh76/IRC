#include "cmds/cmd_join.hpp"
#include "Server.hpp"
#include "Replies.hpp"
#include <cctype>
#include <sstream>
#include <vector>
#include <string>

// 簡單的以逗號分割（IRC JOIN 支援以逗號分隔多個參數）
static std::vector<std::string> splitCSV(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == ',') { out.push_back(cur); cur.clear(); }
        else { cur.push_back(c); }
    }
    out.push_back(cur);
    return out;
}

// 組成 NAMES 回覆內容（只列出 nick） for CmdJoin
static std::string collectNamesFor(Server& srv, const std::string& ch) {
    std::ostringstream oss;
    const Channel& chan = srv.getChannel(ch);
    const std::set<int>& members = chan.members();

    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it) {
        const Client* member = srv.getClientByFd(*it);       // 取得指標
        if (!member) continue;                                // 防禦

    if (it != members.begin()) oss << " ";
    // NAMES reply per RFC: list nicks, optionally prefixed with '@' for ops
    if (chan.isOperator(*it)) oss << "@" << member->getNick();
    else                      oss <<       member->getNick();
    }
    return oss.str();
}

// Execute | Format: JOIN <channel> [key]
// Joins a channel. Checks for +i (invite-only), +k (password), +l (user limit) modes.
void CmdJoin::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }
    if (m.params.empty()) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "JOIN"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 解析多個頻道（與可能的 key 列表）
    const std::vector<std::string> chans = splitCSV(m.params[0]);
    std::vector<std::string> keys;
    if (m.params.size() > 1) keys = splitCSV(m.params[1]);

    for (size_t i = 0; i < chans.size(); ++i) {
        const std::string ch = chans[i];

        if (!isValidChannelName(ch)) {
            cl.sendLine(":" + srv.serverName() + " 476 " + cl.getNick() + " " + ch + " :Bad Channel Mask");
            srv.enableWriteForFd(cl.fd());
            continue;
        }

        bool exists = srv.channelExists(ch);
        if (exists) {
            // 只在已存在時取得頻道（避免誤創建）
            Channel& chan = srv.getChannel(ch);
            std::cout << "[DEBUG] Found existing channel: " << ch << ", mode: " << (chan.isInviteOnly() ? "+i" : "public") << std::endl;

            // 已在頻道內 → 略過
            if (srv.isChannelMember(ch, cl.fd())) {
                continue;
            }

            // +i：必須曾被邀請（用 nick 判斷）
            if (chan.isInviteOnly()) {
                std::string userNick = toLower(cl.getNick());
                if (!chan.isInvitedNick(userNick)) {
                    cl.sendLine(ERR_INVITEONLYCHAN(srv.serverName(), cl.getNick(), ch));
                    srv.enableWriteForFd(cl.fd());
                    continue;
                }
            }

            // +k（頻道密碼）
            if (!chan.key().empty()) {
                const std::string providedKey = (i < keys.size()) ? keys[i] : "";
                if (providedKey != chan.key()) {
                    cl.sendLine(ERR_BADCHANNELKEY(srv.serverName(), cl.getNick(), ch));
                    srv.enableWriteForFd(cl.fd());
                    continue;
                }
            }

            // +l（人數上限）
            if (chan.userLimit() > 0 && chan.members().size() >= chan.userLimit()) {
                cl.sendLine(ERR_CHANNELISFULL(srv.serverName(), cl.getNick(), ch));
                srv.enableWriteForFd(cl.fd());
                continue;
            }

            // 實際加入
            if (srv.addClientToChannel(ch, cl.fd())) {
                if (chan.members().size() == 1) {
                    chan.addOperator(cl.fd());
                }
                if (chan.isInvitedNick(toLower(cl.getNick()))) {
                    chan.clearInviteNick(toLower(cl.getNick()));
                }
                const std::string prefix = cl.getFullPrefix();
                cl.sendLine(prefix + " JOIN :" + ch);
                srv.enableWriteForFd(cl.fd());
                srv.broadcastToChannel(ch, prefix + " JOIN :" + ch, cl.fd());

                if (!chan.getTopic().empty()) {
                    cl.sendLine(RPL_TOPIC(srv.serverName(), ch, chan.getTopic()));
                } else {
                    cl.sendLine(RPL_NOTOPIC(srv.serverName(), ch));
                }
                srv.enableWriteForFd(cl.fd());

                // Send channel creation time (329)
                {
                    std::ostringstream ts;
                    ts << chan.createdAt();
                    cl.sendLine(RPL_CREATIONTIME(srv.serverName(), cl.getNick(), ch, ts.str()));
                    srv.enableWriteForFd(cl.fd());
                }

                std::string names = collectNamesFor(srv, ch);
                cl.sendLine(RPL_NAMREPLY(srv.serverName(), cl.getNick(), ch, names));
                cl.sendLine(RPL_ENDOFNAMES(srv.serverName(), cl.getNick(), ch));
                srv.enableWriteForFd(cl.fd());

                // Bot welcome (NOTICE, avoid channel noise): seed 42/IRC easter eggs
                srv.broadcastToChannel(ch, ":" + srv.serverName() + " NOTICE " + ch + " :Welcome to " + ch + ". need help? Say '!help'.", -1);
                srv.broadcastToChannel(ch, ":" + srv.serverName() + " NOTICE " + ch + " :Pro tip: mention 'IRC' '42' to get a fun fact.", -1);
            }
        } else {
            // 頻道不存在：此次 JOIN 創建並加入（符合 RFC 行為）
            Channel& chan = srv.getChannel(ch);
            if (srv.addClientToChannel(ch, cl.fd())) {
                if (chan.members().size() == 1) {
                    chan.addOperator(cl.fd());
                }
                const std::string prefix = cl.getFullPrefix();
                cl.sendLine(prefix + " JOIN :" + ch);
                srv.enableWriteForFd(cl.fd());
                srv.broadcastToChannel(ch, prefix + " JOIN :" + ch, cl.fd());

                if (!chan.getTopic().empty()) {
                    cl.sendLine(RPL_TOPIC(srv.serverName(), ch, chan.getTopic()));
                } else {
                    cl.sendLine(RPL_NOTOPIC(srv.serverName(), ch));
                }
                srv.enableWriteForFd(cl.fd());

                // Send channel creation time (329)
                {
                    std::ostringstream ts;
                    ts << chan.createdAt();
                    cl.sendLine(RPL_CREATIONTIME(srv.serverName(), cl.getNick(), ch, ts.str()));
                    srv.enableWriteForFd(cl.fd());
                }

                std::string names = collectNamesFor(srv, ch);
                cl.sendLine(RPL_NAMREPLY(srv.serverName(), cl.getNick(), ch, names));
                cl.sendLine(RPL_ENDOFNAMES(srv.serverName(), cl.getNick(), ch));
                srv.enableWriteForFd(cl.fd());

                // Bot welcome (NOTICE, avoid channel noise): seed 42/IRC easter eggs
                srv.broadcastToChannel(ch, ":" + srv.serverName() + " NOTICE " + ch + " :Welcome to " + ch + ". Want to test if I'm alive? Say '42'.", -1);
                srv.broadcastToChannel(ch, ":" + srv.serverName() + " NOTICE " + ch + " :Pro tip: mention 'IRC' to get a fun fact.", -1);
            }
        }
    }
}

