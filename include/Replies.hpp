#ifndef REPLIES_HPP
#define REPLIES_HPP

// ======================= Behavior replies (non-numeric) =======================
// source is usually ":nick" or ":server"
#define RPL_JOIN(source, channel)            (":" + (source) + " JOIN :" + (channel))
#define RPL_PART(source, channel, reason)    (":" + (source) + " PART :" + (channel) + " :" + (reason))
#define RPL_PING(source, token)              (":" + (source) + " PONG :" + (token))
#define RPL_PRIVMSG(source, target, msg)     (":" + (source) + " PRIVMSG " + (target) + " :" + (msg))
#define RPL_NOTICE(source, target, msg)      (":" + (source) + " NOTICE " + (target) + " :" + (msg))
#define RPL_QUIT(source, msg)                (":" + (source) + " QUIT :Quit: " + (msg))
#define RPL_KICK(source, ch, target, reason) (":" + (source) + " KICK " + (ch) + " " + (target) + " :" + (reason))
#define RPL_MODE(source, ch, modes, args)    (":" + (source) + " MODE " + (ch) + " " + (modes) + " " + (args))

// ======================= Registration completion welcome message =======================
#define RPL_WELCOME(server, nick) (":" + (server) + " 001 " + (nick) + " :Welcome to the ft_irc network, " + (nick))
// 329 RPL_CREATIONTIME (channel creation time, timestamp)
#define RPL_CREATIONTIME(server, nick, ch, ts) (":" + (server) + " 329 " + (nick) + " " + (ch) + " " + (ts))

// ======================= Common error numerics =======================
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
// 432 ERR_ERRONEUSNICKNAME (optional)
#define ERR_ERRONEUSNICKNAME(server, nick)   (":" + (server) + " 432 " + (nick) + " :Erroneous nickname")
// 433 ERR_NICKNAMEINUSE
#define ERR_NICKNAMEINUSE(server, nick)      (":" + (server) + " 433 * " + (nick) + " :Nickname is already in use")
// 437 ERR_UNAVAILRESOURCE (optional)
#define ERR_UNAVAILRESOURCE(server, res)     (":" + (server) + " 437 " + (res) + " :Nick/channel is temporarily unavailable")
// 441 ERR_USERNOTINCHANNEL (sometimes used in KICK/PRIVMSG)
#define ERR_USERNOTINCHANNEL(server, nick, ch) (":" + (server) + " 441 " + (nick) + " " + (ch) + " :They aren't on that channel")
// 442 ERR_NOTONCHANNEL
#define ERR_NOTONCHANNEL(server, nick, ch)         (":" + (server) + " 442 " + (nick) + " " + (ch) + " :You're not on that channel")
// 443 ERR_USERONCHANNEL (INVITE)
#define ERR_USERONCHANNEL(server, nick, ch)  (":" + (server) + " 443 " + (nick) + " " + (ch) + " :is already on channel")
// 451 ERR_NOTREGISTERED (using other commands before completing NICK/USER)
#define ERR_NOTREGISTERED(server)            (":" + (server) + " 451 :You have not registered")
// 461 ERR_NEEDMOREPARAMS
#define ERR_NEEDMOREPARAMS(server, cmd)      (":" + (server) + " 461 " + (cmd) + " :Not enough parameters")
// 462 ERR_ALREADYREGISTRED
#define ERR_ALREADYREGISTRED(server)         (":" + (server) + " 462 :You may not reregister")
// 464 ERR_PASSWDMISMATCH (if has PASS)
#define ERR_PASSWDMISMATCH(server)           (":" + (server) + " 464 :Password incorrect")
// 471 ERR_CHANNELISFULL (+l)
#define ERR_CHANNELISFULL(server, nick, ch)        (":" + (server) + " 471 " + (nick) + " " + (ch) + " :Cannot join channel (+l)")
// 473 ERR_INVITEONLYCHAN (+i)
#define ERR_INVITEONLYCHAN(server, nick, ch)       (":" + (server) + " 473 " + (nick) + " " + (ch) + " :Cannot join channel (+i)")
// 474 ERR_BANNEDFROMCHAN (optional)
#define ERR_BANNEDFROMCHAN(server, ch)       (":" + (server) + " 474 " + (ch) + " :Cannot join channel (+b)")
// 475 ERR_BADCHANNELKEY (+k)
#define ERR_BADCHANNELKEY(server, nick, ch)        (":" + (server) + " 475 " + (nick) + " " + (ch) + " :Cannot join channel (+k)")
// 482 ERR_CHANOPRIVSNEEDED (need channel privilege)
#define ERR_CHANOPRIVSNEEDED(server, ch)     (":" + (server) + " 482 " + (ch) + " :You're not channel operator")
// 421 ERR_UNKNOWNCOMMAND (unknown command, optional)
#define ERR_UNKNOWNCOMMAND(server, cmd)      (":" + (server) + " 421 " + (cmd) + " :Unknown command")
// 472 ERR_UNKNOWNMODE (unknown mode character)
#define ERR_UNKNOWNMODE(server, nick, mode)  (":" + (server) + " 472 " + (nick) + " " + (mode) + " :is unknown mode char to me")
// 696 ERR_INVALIDMODEPARAM (invalid mode parameter)
#define ERR_INVALIDMODEPARAM(server, nick, ch, mode, param, desc) (":" + (server) + " 696 " + (nick) + " " + (ch) + " " + (mode) + " " + (param) + " :" + (desc))

// ======================= Common positive numerics (optional/bonus) =======================
// 352 RPL_WHOREPLY
// Format: ":server 352 <me> <channel> <user> <host> <server> <nick> <H|G><@|+> :<hopcount> <realname>"
#define RPL_WHOREPLY(server, me, ch, user, host, srv, nick, flags, hops, real) \
	(":" + (server) + " 352 " + (me) + " " + (ch) + " " + (user) + " " + (host) + " " + (srv) + " " + (nick) + " " + (flags) + " :" + (hops) + " " + (real))
// 315 RPL_ENDOFWHO
#define RPL_ENDOFWHO(server, me, name)      (":" + (server) + " 315 " + (me) + " " + (name) + " :End of WHO list")
// 324 RPL_CHANNELMODEIS (MODE #channel query reply)
#define RPL_CHANNELMODEIS(server, nick, ch, modes, args) (":" + (server) + " 324 " + (nick) + " " + (ch) + " " + (modes) + ((args).empty()? "" : (" " + (args))))
// 331 RPL_NOTOPIC
#define RPL_NOTOPIC(server, ch)              (":" + (server) + " 331 " + (ch) + " :No topic is set")
// 332 RPL_TOPIC
#define RPL_TOPIC(server, ch, topic)         (":" + (server) + " 332 " + (ch) + " :" + (topic))
// 353 RPL_NAMREPLY (simplest format)
#define RPL_NAMREPLY(server, nick, ch, names) (":" + (server) + " 353 " + (nick) + " = " + (ch) + " :" + (names))
// 366 RPL_ENDOFNAMES
#define RPL_ENDOFNAMES(server, nick, ch)     (":" + (server) + " 366 " + (nick) + " " + (ch) + " :End of /NAMES list.")
// 367 RPL_BANLIST (list ban masks)
#define RPL_BANLIST(server, nick, ch, mask, setter, timestamp) (":" + (server) + " 367 " + (nick) + " " + (ch) + " " + (mask) + " " + (setter) + " " + (timestamp))
// 368 RPL_ENDOFBANLIST
#define RPL_ENDOFBANLIST(server, nick, ch)   (":" + (server) + " 368 " + (nick) + " " + (ch) + " :End of channel ban list")
// 346 RPL_INVITELIST (+I list entry)
#define RPL_INVITELIST(server, nick, ch, mask) (":" + (server) + " 346 " + (nick) + " " + (ch) + " " + (mask))
// 347 RPL_ENDOFINVITELIST
#define RPL_ENDOFINVITELIST(server, nick, ch) (":" + (server) + " 347 " + (nick) + " " + (ch) + " :End of channel invite list")
// 348 RPL_EXCEPTLIST (+e list entry)
#define RPL_EXCEPTLIST(server, nick, ch, mask) (":" + (server) + " 348 " + (nick) + " " + (ch) + " " + (mask))
// 349 RPL_ENDOFEXCEPTLIST
#define RPL_ENDOFEXCEPTLIST(server, nick, ch) (":" + (server) + " 349 " + (nick) + " " + (ch) + " :End of channel exception list")

#endif // REPLIES_HPP
