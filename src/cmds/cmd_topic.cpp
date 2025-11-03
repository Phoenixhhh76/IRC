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

    // Check if channel exists
    if (!srv.channelExists(ch)) {
        cl.sendLine(ERR_NOSUCHCHANNEL(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Check if user is in channel
    if (!srv.isChannelMember(ch, cl.fd())) {
    cl.sendLine(ERR_NOTONCHANNEL(srv.serverName(), cl.getNick(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Get channel (need Server to provide getter)
    Channel& chan = srv.getChannel(ch);

    // If no topic parameter provided → query topic
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

    // Topic provided → set topic
    const std::string& newTopic = m.params[1];

    // If channel is +t, check if operator permission needed
    if (chan.isTopicRestricted() && !srv.isChannelOperator(ch, cl.fd())) {
        cl.sendLine(ERR_CHANOPRIVSNEEDED(srv.serverName(), ch));
        srv.enableWriteForFd(cl.fd());
        return;
    }

    // Set topic and broadcast
    chan.setTopic(newTopic);

    // Broadcast TOPIC change to channel members
    std::string topicMsg = cl.getFullPrefix() + " TOPIC " + ch + " :" + newTopic;
    srv.broadcastToChannel(ch, topicMsg, -1);
}
