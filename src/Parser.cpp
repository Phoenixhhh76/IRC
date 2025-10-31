#include "Parser.hpp"
#include <cctype>   // std::toupper
#include <string>

// 去尾端 CR / LF
static std::string rstrip_crlf(const std::string& s) {
    std::string r = s;
    while (!r.empty()) {
        char c = r[r.size() - 1];
        if (c == '\r' || c == '\n') r.erase(r.size() - 1);
        else break;
    }
    return r;
}

// 跳過空白
static void skip_spaces(const std::string& s, size_t& i) {
    while (i < s.size() && s[i] == ' ') ++i;
}

// 就地轉大寫（C++98）
static void to_upper_inplace(std::string& s) {
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
}

IrcMessage parseLine(const std::string& raw) {
    IrcMessage msg;
    std::string s = rstrip_crlf(raw);
    size_t i = 0;

    // 1) prefix（可選）：以 ':' 開頭直到第一個空白
    if (!s.empty() && s[0] == ':') {
        size_t sp = s.find(' ');
        if (sp != std::string::npos) {
            msg.prefix = s.substr(1, sp - 1);
            i = sp + 1;
        } else {
            // 僅有前綴（不太合理），直接回傳空 command 的 msg
            return msg;
        }
    }

    // 2) command：下一段非空白字串
    skip_spaces(s, i);
    size_t cmd_end = i;
    while (cmd_end < s.size() && s[cmd_end] != ' ') ++cmd_end;
    if (cmd_end > i) {
        msg.command = s.substr(i, cmd_end - i);
        to_upper_inplace(msg.command);
    }
    i = cmd_end;

    // 3) params：零到多個「中間參數」，以及一個可選的「trailing」
    //    - middle param：非空白連續字元
    //    - trailing：以 ':' 開頭，之後整串（可含空白）
    while (i < s.size()) {
        skip_spaces(s, i);
        if (i >= s.size()) break;

        if (s[i] == ':') {
            // trailing：吃到尾，丟進 params 的最後一個元素
            msg.params.push_back(s.substr(i + 1));
            break;
        } else {
            // 对于命令参数，只取到第一个空格
            size_t j = i;
            while (j < s.size() && s[j] != ' ') ++j;
            // 提取非空参数
            std::string param = s.substr(i, j - i);
            if (!param.empty()) {
                msg.params.push_back(param);
            }
            i = j;
        }
    }

    return msg;
}
