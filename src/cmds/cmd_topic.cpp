#include "cmds/cmd_topic.hpp"
#include "Server.hpp"
#include "Replies.hpp"

// Execute | Format: TOPIC <channel> [topic]
// Sets or displays the channel topic. Requires channel operator if channel mode is +t.
void CmdTopic::execute(Server& srv, Client& cl, const IrcMessage& m) {
    if (!cl.registered()) {
        cl.sendLine(ERR_NOTREGISTERED(srv.serverName()));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    if (m.params.empty()) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "TOPIC"));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    const std::string& ch = m.params[0];

    // 檢查頻道是否存在
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 檢查用戶是否在頻道
    if (!srv.isChannelMember(ch, cl.fd())) {
    cl.sendLine(ERR_NOTONCHANNEL(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 取得頻道（需要 Server 提供 getter）
    Channel& chan = srv.getChannel(ch);

    // 如果沒有提供主題參數 → 查詢主題
    if (m.params.size() == 1) {
        const std::string& topic = chan.getTopic();
        if (topic.empty()) {
            cl.sendLine(RPL_NOTOPIC(srv.serverName(), ch));
        } else {
            cl.sendLine(RPL_TOPIC(srv.serverName(), ch, topic));
        }
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 提供主題 → 設定主題
    const std::string& newTopic = m.params[1];

    // 若頻道是 +t，檢查是否需要 operator 權限
    if (chan.isTopicRestricted() && !srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // 設定主題並廣播
    chan.setTopic(newTopic);

    // 廣播 TOPIC 變化給頻道成員
    std::string topicMsg = cl.getFullPrefix() + " TOPIC " + ch + " :" + newTopic;
    srv.broadcastToChannel(ch, topicMsg, -1);
}
