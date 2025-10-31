#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <csignal>

// 工具函数：将字符串转换为小写
std::string toLower(const std::string& str);

// RFC 2812 的 NICK 合法性檢查（長度 1..9；首字非數字/'-'；
// 允許英數 + "[]\\`^{}|-"；不允許空白）
bool isValidNick(const std::string& nick);

static inline bool isCtl(unsigned char c);

bool isValidChannelName(const std::string& ch);

// 构建完整的用户前缀 (:nick!user@host)
std::string buildPrefix(const std::string& nick, const std::string& user, const std::string& host);

// 规范化频道名（RFC1459 casemapping）
// - 转小写 A-Z -> a-z
// - 将 '{' 映射为 '['， '}' 映射为 ']'， '|' 映射为 '\\'
// 这样 "#Test", "#test", "#Te{t" 与 "#Te[t" 会被当成同一频道键值。
std::string normalizeChannelName(const std::string& name);

// ====== Graceful shutdown support ======
// Global flag set by signal handlers when SIGINT/SIGTERM received
extern volatile sig_atomic_t g_stopRequested;

// Install signal handlers: ignore SIGPIPE; set SIGINT/SIGTERM to request stop
void installSignalHandlers();

// Query if a stop was requested (set by signal)
bool stopRequested();

#endif // UTILS_HPP
