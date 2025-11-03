#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

struct IrcMessage {
    std::string prefix;              // Optional, like ":nick!user@host"
    std::string command;             // Converted to uppercase, like "NICK" / "USER" / "PRIVMSG"
    std::vector<std::string> params; // middle params + (trailing also placed here)
};

// Parse one line of IRC text (may contain \r\n) into IrcMessage
IrcMessage parseLine(const std::string& line);

#endif // PARSER_HPP
