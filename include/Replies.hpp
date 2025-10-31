// #ifndef REPLIES_HPP
// #define REPLIES_HPP

// //還有其他需要補，例如其他的Error response，另外防止展開時意外，變數用()保護

// // —— 行為回覆（來源通常是 servername 或 :nick!user@host） ——
// #define RPL_JOIN(source, channel)            (":" + (source) + " JOIN :" + (channel))
// #define RPL_PART(source, channel)            (":" + (source) + " PART :" + (channel))
// #define RPL_PING(source, token)              (":" + (source) + " PONG :" + (token))                 // PONG
// #define RPL_PRIVMSG(source, target, msg)     (":" + (source) + " PRIVMSG " + (target) + " :" + (msg))
// #define RPL_NOTICE(source, target, msg)      (":" + (source) + " NOTICE " + (target) + " :" + (msg))
// #define RPL_QUIT(source, msg)                (":" + (source) + " QUIT :Quit: " + (msg))
// #define RPL_KICK(source, ch, target, reason) (":" + (source) + " KICK " + (ch) + " " + (target) + " :" + (reason))
// #define RPL_MODE(source, ch, modes, args)    (":" + (source) + " MODE " + (ch) + " " + (modes) + " " + (args))

// // —— 註冊階段 numeric replies ——
// #define RPL_WELCOME(server, nick)        (":" + (server) + " 001 " + (nick) + " :Welcome to the ft_irc network, " + (nick))
// #define ERR_NONICKNAMEGIVEN(server)      (":" + (server) + " 431 :No nickname given")
// #define ERR_NICKNAMEINUSE(server, nick)  (":" + (server) + " 433 * " + (nick) + " :Nickname is already in use")
// #define ERR_NEEDMOREPARAMS(server, cmd)  (":" + (server) + " 461 " + (cmd) + " :Not enough parameters")
// #define ERR_ALREADYREGISTRED(server)     (":" + (server) + " 462 :You may not reregister")

// #endif

#ifndef REPLIES_HPP
#define REPLIES_HPP

// ======================= 行為回覆（非 numeric） =======================
// source 通常是 ":nick" 或 ":server"
#define RPL_JOIN(source, channel)            (":" + (source) + " JOIN :" + (channel))
#define RPL_PART(source, channel, reason)    (":" + (source) + " PART :" + (channel) + " :" + (reason))
#define RPL_PING(source, token)              (":" + (source) + " PONG :" + (token))
#define RPL_PRIVMSG(source, target, msg)     (":" + (source) + " PRIVMSG " + (target) + " :" + (msg))
#define RPL_NOTICE(source, target, msg)      (":" + (source) + " NOTICE " + (target) + " :" + (msg))
#define RPL_QUIT(source, msg)                (":" + (source) + " QUIT :Quit: " + (msg))
#define RPL_KICK(source, ch, target, reason) (":" + (source) + " KICK " + (ch) + " " + (target) + " :" + (reason))
#define RPL_MODE(source, ch, modes, args)    (":" + (source) + " MODE " + (ch) + " " + (modes) + " " + (args))

// ======================= 註冊完成歡迎訊息 =======================
#define RPL_WELCOME(server, nick) (":" + (server) + " 001 " + (nick) + " :Welcome to the ft_irc network, " + (nick))
// 329 RPL_CREATIONTIME（頻道創建時間，時間戳）
#define RPL_CREATIONTIME(server, nick, ch, ts) (":" + (server) + " 329 " + (nick) + " " + (ch) + " " + (ts))

// ======================= 常見錯誤 numerics =======================
// 401 ERR_NOSUCHNICK
#define ERR_NOSUCHNICK(server, nick)         (":" + (server) + " 401 " + (nick) + " :No such nick")
// 403 ERR_NOSUCHCHANNEL
#define ERR_NOSUCHCHANNEL(server, ch)        (":" + (server) + " 403 " + (ch) + " :No such channel")
// 404 ERR_CANNOTSENDTOCHAN
#define ERR_CANNOTSENDTOCHAN(server, nick, ch)     (":" + (server) + " 404 " + (nick) + " " + (ch) + " :Cannot send to channel")
// 412 ERR_NOTEXTTOSEND
#define ERR_NOTEXTTOSEND(server)             (":" + (server) + " 412 :No text to send")
// 431 ERR_NONICKNAMEGIVEN
#define ERR_NONICKNAMEGIVEN(server)          (":" + (server) + " 431 :No nickname given")
// 432 ERR_ERRONEUSNICKNAME（可選）
#define ERR_ERRONEUSNICKNAME(server, nick)   (":" + (server) + " 432 " + (nick) + " :Erroneous nickname")
// 433 ERR_NICKNAMEINUSE
#define ERR_NICKNAMEINUSE(server, nick)      (":" + (server) + " 433 * " + (nick) + " :Nickname is already in use")
// 437 ERR_UNAVAILRESOURCE（可選）
#define ERR_UNAVAILRESOURCE(server, res)     (":" + (server) + " 437 " + (res) + " :Nick/channel is temporarily unavailable")
// 441 ERR_USERNOTINCHANNEL（KICK/PRIVMSG 有時會用到）
#define ERR_USERNOTINCHANNEL(server, nick, ch) (":" + (server) + " 441 " + (nick) + " " + (ch) + " :They aren't on that channel")
// 442 ERR_NOTONCHANNEL
#define ERR_NOTONCHANNEL(server, nick, ch)         (":" + (server) + " 442 " + (nick) + " " + (ch) + " :You're not on that channel")
// 443 ERR_USERONCHANNEL（INVITE）
#define ERR_USERONCHANNEL(server, nick, ch)  (":" + (server) + " 443 " + (nick) + " " + (ch) + " :is already on channel")
// 451 ERR_NOTREGISTERED（尚未完成 NICK/USER 就用其他指令）
#define ERR_NOTREGISTERED(server)            (":" + (server) + " 451 :You have not registered")
// 461 ERR_NEEDMOREPARAMS
#define ERR_NEEDMOREPARAMS(server, cmd)      (":" + (server) + " 461 " + (cmd) + " :Not enough parameters")
// 462 ERR_ALREADYREGISTRED
#define ERR_ALREADYREGISTRED(server)         (":" + (server) + " 462 :You may not reregister")
// 464 ERR_PASSWDMISMATCH（如有 PASS）
#define ERR_PASSWDMISMATCH(server)           (":" + (server) + " 464 :Password incorrect")
// 471 ERR_CHANNELISFULL（+l）
#define ERR_CHANNELISFULL(server, nick, ch)        (":" + (server) + " 471 " + (nick) + " " + (ch) + " :Cannot join channel (+l)")
// 473 ERR_INVITEONLYCHAN（+i）
#define ERR_INVITEONLYCHAN(server, nick, ch)       (":" + (server) + " 473 " + (nick) + " " + (ch) + " :Cannot join channel (+i)")
// 474 ERR_BANNEDFROMCHAN（可選）
#define ERR_BANNEDFROMCHAN(server, ch)       (":" + (server) + " 474 " + (ch) + " :Cannot join channel (+b)")
// 475 ERR_BADCHANNELKEY（+k）
#define ERR_BADCHANNELKEY(server, nick, ch)        (":" + (server) + " 475 " + (nick) + " " + (ch) + " :Cannot join channel (+k)")
// 482 ERR_CHANOPRIVSNEEDED（需要管道權限）
#define ERR_CHANOPRIVSNEEDED(server, ch)     (":" + (server) + " 482 " + (ch) + " :You're not channel operator")
// 421 ERR_UNKNOWNCOMMAND（未知指令，可選）
#define ERR_UNKNOWNCOMMAND(server, cmd)      (":" + (server) + " 421 " + (cmd) + " :Unknown command")
// 472 ERR_UNKNOWNMODE（未知模式字元）
#define ERR_UNKNOWNMODE(server, nick, mode)  (":" + (server) + " 472 " + (nick) + " " + (mode) + " :is unknown mode char to me")
// 696 ERR_INVALIDMODEPARAM（無效模式參數）
#define ERR_INVALIDMODEPARAM(server, nick, ch, mode, param, desc) (":" + (server) + " 696 " + (nick) + " " + (ch) + " " + (mode) + " " + (param) + " :" + (desc))

// ======================= 常見正向 numerics（可選/加分） =======================
// 352 RPL_WHOREPLY
// Format: ":server 352 <me> <channel> <user> <host> <server> <nick> <H|G><@|+> :<hopcount> <realname>"
#define RPL_WHOREPLY(server, me, ch, user, host, srv, nick, flags, hops, real) \
	(":" + (server) + " 352 " + (me) + " " + (ch) + " " + (user) + " " + (host) + " " + (srv) + " " + (nick) + " " + (flags) + " :" + (hops) + " " + (real))
// 315 RPL_ENDOFWHO
#define RPL_ENDOFWHO(server, me, name)      (":" + (server) + " 315 " + (me) + " " + (name) + " :End of WHO list")
// 324 RPL_CHANNELMODEIS (MODE #channel 查詢回覆)
#define RPL_CHANNELMODEIS(server, nick, ch, modes, args) (":" + (server) + " 324 " + (nick) + " " + (ch) + " " + (modes) + ((args).empty()? "" : (" " + (args))))
// 331 RPL_NOTOPIC
#define RPL_NOTOPIC(server, ch)              (":" + (server) + " 331 " + (ch) + " :No topic is set")
// 332 RPL_TOPIC
#define RPL_TOPIC(server, ch, topic)         (":" + (server) + " 332 " + (ch) + " :" + (topic))
// 353 RPL_NAMREPLY（最簡格式）
#define RPL_NAMREPLY(server, nick, ch, names) (":" + (server) + " 353 " + (nick) + " = " + (ch) + " :" + (names))
// 366 RPL_ENDOFNAMES
#define RPL_ENDOFNAMES(server, nick, ch)     (":" + (server) + " 366 " + (nick) + " " + (ch) + " :End of /NAMES list.")
// 367 RPL_BANLIST（列出封禁面具）
#define RPL_BANLIST(server, nick, ch, mask, setter, timestamp) (":" + (server) + " 367 " + (nick) + " " + (ch) + " " + (mask) + " " + (setter) + " " + (timestamp))
// 368 RPL_ENDOFBANLIST
#define RPL_ENDOFBANLIST(server, nick, ch)   (":" + (server) + " 368 " + (nick) + " " + (ch) + " :End of channel ban list")
// 346 RPL_INVITELIST（+I 列表條目）
#define RPL_INVITELIST(server, nick, ch, mask) (":" + (server) + " 346 " + (nick) + " " + (ch) + " " + (mask))
// 347 RPL_ENDOFINVITELIST
#define RPL_ENDOFINVITELIST(server, nick, ch) (":" + (server) + " 347 " + (nick) + " " + (ch) + " :End of channel invite list")
// 348 RPL_EXCEPTLIST（+e 列表條目）
#define RPL_EXCEPTLIST(server, nick, ch, mask) (":" + (server) + " 348 " + (nick) + " " + (ch) + " " + (mask))
// 349 RPL_ENDOFEXCEPTLIST
#define RPL_ENDOFEXCEPTLIST(server, nick, ch) (":" + (server) + " 349 " + (nick) + " " + (ch) + " :End of channel exception list")

#endif // REPLIES_HPP
