#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

struct IrcMessage {
    std::string prefix;              // 可空，如 ":nick!user@host"
    std::string command;             // 轉為大寫，如 "NICK" / "USER" / "PRIVMSG"
    std::vector<std::string> params; // middle params + (最後的 trailing 也置於此)
};

// 將一行 IRC 文本（可能含 \r\n）解析為 IrcMessage
IrcMessage parseLine(const std::string& line);

#endif // PARSER_HPP
