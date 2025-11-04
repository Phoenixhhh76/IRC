#include "utils.hpp"
#include <algorithm>
#include <cctype>

// Implementation: convert string to lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// RFC 2812 nickname validation
bool isValidNick(const std::string& nick) {
    // 1. Check length
    if (nick.empty() || nick.length() > 9)
        return false;

    // 2. Check first character
    if (isdigit(nick[0]) || nick[0] == '-')
        return false;

    // 3. Check all characters
    for (size_t i = 0; i < nick.length(); ++i) {
        char c = nick[i];
        // No spaces or other whitespace
        if (isspace(c))
            return false;
        // Only allow letters, digits and specific special characters
        if (!(isalnum(c) ||  // Letter or digit
              c == '-' || c == '[' || c == ']' || c == '\\' ||
              c == '`' || c == '^' || c == '{' || c == '|' || c == '}'))
            return false;
    }

    return true;
}

bool isValidChannelName(const std::string& ch) {
    if (ch.size() < 2 || ch.size() > 50) return false;   // Length
    if (ch[0] != '#') return false;                      // Prefix
    for (size_t i = 1; i < ch.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(ch[i]);
        if (c == ',' || c == '\a' || isCtl(c) || c == ' ') return false;
        // Accept other characters (you can narrow the whitelist if needed)
    }
    return true;
}

// Build complete user prefix (:nick!user@host)
std::string buildPrefix(const std::string& nick, const std::string& user, const std::string& host) {
    std::string prefix = ":" + (!nick.empty() ? nick : "*");
    prefix += "!" + (!user.empty() ? user : "unknown");
    prefix += "@" + (!host.empty() ? host : "unknown");
    return prefix;
}

// Validate server password according to security rules
bool isValidPassword(const std::string& password) {
    // 1. Check length: minimum 3 characters, maximum 50 characters
    if (password.length() < 3 || password.length() > 50) {
        return false;
    }

    // 2. Check each character
    for (size_t i = 0; i < password.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(password[i]);

        // Must be ASCII printable (32-126), excluding whitespace
        if (c < 33 || c > 126) {
            return false;
        }

        // No whitespace characters (space, tab, newline, etc.)
        if (isspace(c)) {
            return false;
        }
    }

    return true;
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
