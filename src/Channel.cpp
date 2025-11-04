#include "Channel.hpp"
#include "utils.hpp"

// ∗ KICK - Eject a client from the channel - removeClient
// ∗ INVITE - Invite a client to a channel - addClient
// ∗ TOPIC - Change or view the channel topic - setTopic, getTopic
// ∗ MODE - Change the channel’s mode:
// · i: Set/remove Invite-only channel - setInviteOnly
// · t: Set/remove the restrictions of the TOPIC command to channel
// operators - setTopicRestricted
// · k: Set/remove the channel key (password) - setKey
// · o: Give/take channel operator privilege - addOperator, removeOperator
// · l: Set/remove the user limit to channel - setUserLimit


Channel::Channel(const std::string& name) {
	_name = name;
	_topic = "";
	_key = "";
	_userLimit = 0;
	_isInviteOnly = false;
	_topicRestricted = false;
    _createdAt = std::time(NULL);
}

const std::string& Channel::getName() const {
	return _name;
}

const std::string& Channel::getTopic() const {
	return _topic;
}

void Channel::setTopic(const std::string& topic) {
	_topic = topic;
}

void Channel::setInviteOnly(bool inviteOnly) {
    _isInviteOnly = inviteOnly;
    //std::cout << " Channel set to " << (inviteOnly ? "invite-only" : "public") << std::endl;
    //std::cout << "[IRC] Current invite list:" << std::endl;
    for (std::set<std::string>::const_iterator it = _invitedNicks.begin(); it != _invitedNicks.end(); ++it) {
        //std::cout << "  - " << *it << std::endl;
    }
}

bool Channel::isInviteOnly() const { return _isInviteOnly; }

void Channel::setTopicRestricted(bool restricted) {
	_topicRestricted = restricted;
}

bool Channel::isTopicRestricted() const { return _topicRestricted; }

void Channel::setKey(const std::string& key) {
	_key = key;
}

const std::string& Channel::key() const { return _key; }

void Channel::setUserLimit(size_t limit) {
	_userLimit = limit;
}

size_t Channel::userLimit() const { return _userLimit; }

bool Channel::addClient(int fd) { return _clients.insert(fd).second; }

void Channel::removeClient(int fd) {
    _clients.erase(fd);
    _operators.erase(fd);
    //_invitedClients.erase(fd);
}

bool Channel::hasClient(int fd) const {
	return _clients.find(fd) != _clients.end();
}


void Channel::addOperator(int fd) {
    if (hasClient(fd)) {  // Only add operator status if they are in the channel
        _operators.insert(fd);
    }
}

void Channel::removeOperator(int fd) { _operators.erase(fd); }
bool Channel::isOperator(int fd) const { return _operators.find(fd) != _operators.end(); }

void Channel::inviteNick(const std::string& nick) {
    std::string loweredNick = toLower(nick);
    _invitedNicks.insert(loweredNick);
    //for (std::set<std::string>::const_iterator it = _invitedNicks.begin(); it != _invitedNicks.end(); ++it) {
       // std::cout << "  - " << *it << std::endl;}
}

bool Channel::isInvitedNick(const std::string& nick) const {
    std::string loweredNick = toLower(nick);
    //std::cout << "[IRC] Channel " << _name << "::isInvitedNick - Checking for: " << loweredNick << std::endl;
    //std::cout << "[IRC] Channel " << _name << " current invited users:" << std::endl;
    //for (std::set<std::string>::const_iterator it = _invitedNicks.begin(); it != _invitedNicks.end(); ++it) {
        //std::cout << "  - " << *it << std::endl;}
    bool found = _invitedNicks.find(loweredNick) != _invitedNicks.end();
    //std::cout << "[IRC] Is invited to " << _name << ": " << (found ? "yes" : "no") << std::endl;
    return found;
}

void Channel::clearInviteNick(const std::string& nick) {
    std::string loweredNick = toLower(nick);
    //std::cout << "[IRC] Channel::clearInviteNick - Removing invite for: " << loweredNick << std::endl;
    _invitedNicks.erase(loweredNick);
}

const std::set<int>& Channel::members() const { return _clients; }