#include "utils.hpp"
#include <algorithm>
#include <cctype>

// 实现：将字符串转换为小写
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// RFC 2812规范的昵称验证
bool isValidNick(const std::string& nick) {
    // 1. 检查长度
    if (nick.empty() || nick.length() > 9)
        return false;

    // 2. 检查首字符
    if (isdigit(nick[0]) || nick[0] == '-')
        return false;

    // 3. 检查所有字符
    for (size_t i = 0; i < nick.length(); ++i) {
        char c = nick[i];
        // 不允许空格和其他空白字符
        if (isspace(c))
            return false;
        // 只允许字母、数字和特定的特殊字符
        if (!(isalnum(c) ||  // 字母或数字
              c == '-' || c == '[' || c == ']' || c == '\\' ||
              c == '`' || c == '^' || c == '{' || c == '|' || c == '}'))
            return false;
    }

    return true;
}

static inline bool isCtl(unsigned char c){ return c < 0x20 || c == 0x7F; }

bool isValidChannelName(const std::string& ch) {
    if (ch.size() < 2 || ch.size() > 50) return false;   // 長度
    if (ch[0] != '#') return false;                      // 前綴
    for (size_t i = 1; i < ch.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(ch[i]);
        if (c == ',' || c == '\a' || isCtl(c) || c == ' ') return false;
        // 其餘字元接受（你也可收窄白名單）
    }
    return true;
}

// 构建完整的用户前缀 (:nick!user@host)
std::string buildPrefix(const std::string& nick, const std::string& user, const std::string& host) {
    std::string prefix = ":" + (!nick.empty() ? nick : "*");
    prefix += "!" + (!user.empty() ? user : "unknown");
    prefix += "@" + (!host.empty() ? host : "unknown");
    return prefix;
}

// ====== Graceful shutdown support ======
volatile sig_atomic_t g_stopRequested = 0;

static void sig_request_stop_handler(int signo) {
    (void)signo;
    g_stopRequested = 1;
}

void installSignalHandlers() {
    // Ignore SIGPIPE so send() on closed sockets doesn't kill the process
    std::signal(SIGPIPE, SIG_IGN);
    // On Ctrl+C or kill TERM, request a graceful stop; poll will get EINTR
    std::signal(SIGINT,  sig_request_stop_handler);
    std::signal(SIGTERM, sig_request_stop_handler);
}

bool stopRequested() { return g_stopRequested != 0; }

// RFC1459 casemapping for channel names
// Lowercase A-Z and map braces and pipe to their RFC1459 equivalents ("{"->"[", "}"->"]", "|"->"backslash").
std::string normalizeChannelName(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<unsigned char>(c - 'A' + 'a');
        } else if (c == '{') {
            c = '[';
        } else if (c == '}') {
            c = ']';
        } else if (c == '|') {
            c = '\\';
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}
