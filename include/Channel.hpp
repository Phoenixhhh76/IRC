#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <cstddef>
#include <string>
#include <iostream>
#include <set>
#include <cctype>
#include <ctime>


class Channel {

private:
    std::string _name;
    std::string _topic;
    std::string _key;
    size_t      _userLimit;
    bool        _isInviteOnly;
    bool        _topicRestricted;
    std::time_t _createdAt; // channel creation time (epoch seconds)

    std::set<int> _clients; //直接存fd
    std::set<int> _operators;
    //std::set<int> _invitedClients;
    std::set<std::string>   _invitedNicks; //改成存nick (try to fix kick/invite issue)
public:
    explicit Channel(const std::string& name);

    const std::string& getName() const;
    const std::string& getTopic() const;
    void setTopic(const std::string& topic);
    std::time_t createdAt() const { return _createdAt; }

    void setInviteOnly(bool inviteOnly);
    bool isInviteOnly() const;
    void setTopicRestricted(bool restricted);
    bool isTopicRestricted() const;
    void setKey(const std::string& key);
    const std::string& key() const;
    void setUserLimit(size_t limit);
    size_t userLimit() const;


    bool addClient(int fd);
    void removeClient(int fd);
    bool hasClient(int fd) const;

    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;

    void inviteNick(const std::string& nick);
    bool isInvitedNick(const std::string& nick) const;
    void clearInviteNick(const std::string& nick);

    //void broadcastMessage(const std::string& message, Client* sender) const;
    const std::set<int>& members() const;//這邊改成列舉成員，讓Server負責broadcastMessage

};


#endif