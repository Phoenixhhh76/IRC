#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <csignal>

// Utility function: convert string to lowercase
std::string toLower(const std::string& str);

// RFC 2812 NICK validity check (length 1..9; first char not digit/'-';
// Allow alphanumeric + "[]\\`^{}|-"; no whitespace)
bool isValidNick(const std::string& nick);

static inline bool isCtl(unsigned char c) { return c < 0x20 || c == 0x7F; }

bool isValidChannelName(const std::string& ch);

// Build complete user prefix (:nick!user@host)
std::string buildPrefix(const std::string& nick, const std::string& user, const std::string& host);

// Validate server password according to security rules
// Rules: 3-50 characters, ASCII printable only, no whitespace
bool isValidPassword(const std::string& password);

// Normalize channel name (RFC1459 casemapping)
// - Lowercase A-Z -> a-z
// - Map '{' to '[', '}' to ']', '|' to '\\'
// So "#Test", "#test", "#Te{t" and "#Te[t" are treated as same channel key.
std::string normalizeChannelName(const std::string& name);

// ====== Graceful shutdown support ======
// Global flag set by signal handlers when SIGINT/SIGTERM received
extern volatile sig_atomic_t g_stopRequested;

// Install signal handlers: ignore SIGPIPE; set SIGINT/SIGTERM to request stop
void installSignalHandlers();

// Query if a stop was requested (set by signal)
bool stopRequested();

#endif // UTILS_HPP
