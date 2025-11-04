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

    // If target is channel
    if (target[0] == '#') {
        handleChannelMode(srv, cl, m);
    } else {
        // User mode handling
        handleUserMode(srv, cl, m);
    }
}

void CmdMode::handleChannelMode(Server& srv, Client& cl, const IrcMessage& m) {
    const std::string& ch = m.params[0];

    // Check if channel exists
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Get channel
    Channel& chan = srv.getChannel(ch);

    // If no mode provided → query current modes
    if (m.params.size() == 1) {
        std::ostringstream oss;
        oss << "+";
        if (chan.isInviteOnly()) oss << "i";
        if (chan.isTopicRestricted()) oss << "t";
        if (!chan.key().empty()) oss << "k";
        if (chan.userLimit() > 0) oss << "l";

        // Reply to query with 324 numeric per spec: ":server 324 <nick> <#chan> +modes [args]"
        cl.sendLine(RPL_CHANNELMODEIS(srv.serverName(), cl.getNick(), ch, oss.str(), std::string()));
        // Also send 329 channel creation time, improve client display completeness
        {
            std::ostringstream ts;
            ts << chan.createdAt();
            cl.sendLine(RPL_CREATIONTIME(srv.serverName(), cl.getNick(), ch, ts.str()));
        }
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // irssi often sends MODE #chan b/e/I to query lists (ban/except/invite-exempt) — this implementation ends with empty list
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
    if (modes.empty()) return; // Invalid, ignore
    bool add = (modes[0] == '+');
    if (modes.size() < 2) {
        // e.g. single char like "b" already handled in upper layer; other short strings not supported, ignore
        return;
    }
    char mode = modes[1];

    // Check if user is channel operator (only operator can set modes)
    if (!srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Apply mode change
    applyMode(srv, cl, chan, mode, add, m);

    // Broadcast MODE change, use full user prefix to avoid missing host
    std::string args = (m.params.size() > 2 ? m.params[2] : "");
    srv.broadcastToChannel(ch, cl.getFullPrefix() + " MODE " + ch + " " + modes + (args.empty() ? "" : (" " + args)), -1);
}

void CmdMode::applyMode(Server& srv, Client& cl, Channel& chan, char mode, bool add, const IrcMessage& m) {
    int targetFd = -1;
    const std::string& ch = m.params[0];
    switch (mode) {
        case 'i': // Invite-only
            //std::cout << "[IRC] Setting channel mode +i (invite-only): " << (add ? "ON" : "OFF") << std::endl;
            chan.setInviteOnly(add);
            //std::cout << "[DEBUG] Channel is now " << (chan.isInviteOnly() ? "invite-only" : "not invite-only") << std::endl;
            break;
        case 't': // Topic restricted
            chan.setTopicRestricted(add);
            break;
        case 'k': // Channel key
            if (add && m.params.size() >= 3) {
                chan.setKey(m.params[2]);
            } else if (add && m.params.size() < 3) {
                // +k requires a parameter
                cl.sendLine(ERR_INVALIDMODEPARAM(srv.serverName(), cl.getNick(), ch, "k", "*", "Key parameter required"));
                srv.enableWriteForFd(cl.fd());
                return;
            } else if (!add) {
                chan.setKey("");
            }
            break;
        case 'l': // User limit
            if (add && m.params.size() >= 3) {
                // Parse the limit parameter
                const std::string& limitStr = m.params[2];

                // Check for empty parameter
                if (limitStr.empty()) {
                    cl.sendLine(ERR_INVALIDMODEPARAM(srv.serverName(), cl.getNick(), ch, "l", limitStr, "Parameter cannot be empty"));
                    srv.enableWriteForFd(cl.fd());
                    return;
                }

                // Parse the number
                char* endptr;
                long limit = strtol(limitStr.c_str(), &endptr, 10);

                // Check if the parameter is a valid number
                if (*endptr != '\0') {
                    cl.sendLine(ERR_INVALIDMODEPARAM(srv.serverName(), cl.getNick(), ch, "l", limitStr, "Invalid number format"));
                    srv.enableWriteForFd(cl.fd());
                    return;
                }

                // Check for negative numbers
                if (limit < 0) {
                    cl.sendLine(ERR_INVALIDMODEPARAM(srv.serverName(), cl.getNick(), ch, "l", limitStr, "Limit cannot be negative"));
                    srv.enableWriteForFd(cl.fd());
                    return;
                }

                // Set the limit (0 means no limit, which effectively disables +l)
                chan.setUserLimit(static_cast<size_t>(limit));
            } else if (add && m.params.size() < 3) {
                // +l requires a parameter
                cl.sendLine(ERR_INVALIDMODEPARAM(srv.serverName(), cl.getNick(), ch, "l", "*", "Limit parameter required"));
                srv.enableWriteForFd(cl.fd());
                return;
            } else if (!add) {
                chan.setUserLimit(0);  // -l removes the limit
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
            // Unknown mode, send error message
            {
                std::string modeStr(1, mode);
                cl.sendLine(ERR_UNKNOWNMODE(srv.serverName(), cl.getNick(), modeStr));
                srv.enableWriteForFd(cl.fd());
            }
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
