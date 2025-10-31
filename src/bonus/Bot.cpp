#include "Server.hpp"
#include "Channel.hpp"
#include <sstream>
#include <ctime>
#include <vector>
#include <cstdlib>
#include <algorithm>

void Server::initBot() {
    std::srand(std::time(NULL));
    std::cout << "[Bot] Initialized bot: " << _botNick << std::endl;
}

void Server::handleBotMessage(const std::string& senderNick, const std::string& target, const std::string& message) {
    // 被動關鍵字回覆（大小寫不敏感）：42 / irc
    std::string lower = message;
    for (size_t i = 0; i < lower.size(); ++i) lower[i] = std::tolower(lower[i]);
    if (lower.find("42") != std::string::npos) {
        std::vector<std::string> resp42;
        resp42.push_back("42 — the answer to life, the universe, and everything.");
        resp42.push_back("42 — not just a number, but a mindset.");
        resp42.push_back("42 — where logic meets creativity.");
        resp42.push_back("42 — a number, a school, and perhaps a philosophy of persistence.");
        const std::string msg = resp42[std::rand() % resp42.size()];
        botSendMessage(target, msg);
        return;
    } else if (lower.find("irc") != std::string::npos) {
        std::vector<std::string> respIRC;
        respIRC.push_back("IRC — a quiet place where code and conversation intertwine.");
        respIRC.push_back("IRC — timeless, text-based, and still alive.");
        respIRC.push_back("IRC — the backbone of all real-time wisdom.");
        respIRC.push_back("IRC — an ancient network that still rewards those who listen closely.");
        const std::string msg = respIRC[std::rand() % respIRC.size()];
        botSendMessage(target, msg);
        return;
    }

    if (message == "!hello") {
        botSendMessage(target, "Greetings, " + senderNick + ". I'm ft_irc_Bot.");
        botSendMessage(target, "Try: !help, !ping, !time, !stats, !info");
        botSendMessage(target, "You can also ask: !ask <your question>");
    } else if (message == "!ping") {
        botSendMessage(target, "Pong. Yes, still alive.");
    } else if (message == "!info") {
        botSendMessage(target, "This is " + _servername + ": a tiny, non-blocking ft_irc server.");
    } else if (message == "!help") {
        botSendMessage(target, "Greetings. I am ft_irc_Bot — your automated assistant on this network.");
        botSendMessage(target, "I can respond to a few basic commands:");
        botSendMessage(target, "  • !ping — check if I'm alive");
        botSendMessage(target, "  • !time — show current server time");
        botSendMessage(target, "  • !info — short note about this IRC server");
        botSendMessage(target, "  • !ask <question> — ask me something");
        botSendMessage(target, "  • !help — display this message again");
        botSendMessage(target, "I'm here 24/7 — polite, efficient, and slightly sarcastic when necessary.");
        botSendMessage(target, "Additionally: mentioning '42' or 'IRC' may unlock some… trivia.");
    } else if (message.size() > 5 && message.substr(0,5) == "!ask ") {
        std::string q = message.substr(5);
        if (q.size() > 256) q = q.substr(0,256) + "...";
        botSendMessage(target, "Good question: " + q);
        botSendMessage(target, "I'm a tiny bot. Check !help, or ask humans in the channel.");
    } else if (message == "!time") {
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, 80, "Current time: %Y-%m-%d %H:%M:%S", timeinfo);
        botSendMessage(target, std::string(buffer));
    } else if (message == "!stats") {
        size_t userCount = _clients.size();
        size_t channelCount = _channels.size();
        std::ostringstream oss;
        oss << "Server Stats - Users: " << userCount << ", Channels: " << channelCount;
        botSendMessage(target, oss.str());
    }
}

void Server::botSendMessage(const std::string& target, const std::string& message) {
    std::string response = ":" + _botNick + "!bot@ft_irc PRIVMSG " + target + " :" + message;

    if (target.size() > 0 && target[0] == '#') {
        const std::string key = normalizeChannelName(target);
        std::map<std::string, Channel>::iterator it = _channels.find(key);
        if (it != _channels.end()) {
            const std::set<int>& mem = it->second.members();
            for (std::set<int>::const_iterator m = mem.begin(); m != mem.end(); ++m) {
                int fd = *m;
                std::map<int, Client*>::iterator c = _clients.find(fd);
                if (c != _clients.end() && c->second) {
                    c->second->sendLine(response);
                    enableWriteForFd(fd);
                }
            }
        }
    } else {
        sendToNick(target, response);
    }
}

void Server::botSendNotice(const std::string& nick, const std::string& message) {
    std::string line = ":" + _botNick + "!bot@ft_irc NOTICE " + nick + " :" + message;
    sendToNick(nick, line);
}


