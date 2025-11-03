#include "Parser.hpp"
#include <cctype>   // std::toupper
#include <string>

// Strip trailing CR / LF
static std::string rstrip_crlf(const std::string& s) {
    std::string r = s;
    while (!r.empty()) {
        char c = r[r.size() - 1];
        if (c == '\r' || c == '\n') r.erase(r.size() - 1);
        else break;
    }
    return r;
}

// Skip whitespace
static void skip_spaces(const std::string& s, size_t& i) {
    while (i < s.size() && s[i] == ' ') ++i;
}

// Convert to uppercase in-place (C++98)
static void to_upper_inplace(std::string& s) {
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
}

IrcMessage parseLine(const std::string& raw) {
    IrcMessage msg;
    std::string s = rstrip_crlf(raw);
    size_t i = 0;

    // 1) prefix (optional): starts with ':' until first whitespace
    if (!s.empty() && s[0] == ':') {
        size_t sp = s.find(' ');
        if (sp != std::string::npos) {
            msg.prefix = s.substr(1, sp - 1);
            i = sp + 1;
        } else {
            // Only prefix (not reasonable), return msg with empty command
            return msg;
        }
    }

    // 2) command: next non-whitespace string
    skip_spaces(s, i);
    size_t cmd_end = i;
    while (cmd_end < s.size() && s[cmd_end] != ' ') ++cmd_end;
    if (cmd_end > i) {
        msg.command = s.substr(i, cmd_end - i);
        to_upper_inplace(msg.command);
    }
    i = cmd_end;

    // 3) params: zero or more "middle params" and one optional "trailing"
    //    - middle param: consecutive non-whitespace characters
    //    - trailing: starts with ':', entire rest of string (can have spaces)
    while (i < s.size()) {
        skip_spaces(s, i);
        if (i >= s.size()) break;

        if (s[i] == ':') {
            // trailing: consume to end, put in last params element
            msg.params.push_back(s.substr(i + 1));
            break;
        } else {
            // For command parameters, only take until first space
            size_t j = i;
            while (j < s.size() && s[j] != ' ') ++j;
            // Extract non-empty parameter
            std::string param = s.substr(i, j - i);
            if (!param.empty()) {
                msg.params.push_back(param);
            }
            i = j;
        }
    }

    return msg;
}
