#include "CommandInit.hpp"

// 集中 include 所有指令不然都放在Server.cpp裡面很恐怖
#include "cmds/cmd_nick.hpp"
#include "cmds/cmd_user.hpp"
#include "cmds/cmd_ping.hpp"
#include "cmds/cmd_join.hpp"
#include "cmds/cmd_part.hpp"
#include "cmds/cmd_privmsg.hpp"
#include "cmds/cmd_pass.hpp"
#include "cmds/cmd_kick.hpp"
#include "cmds/cmd_invite.hpp"
#include "cmds/cmd_mode.hpp"
#include "cmds/cmd_topic.hpp"
#include "cmds/cmd_quit.hpp"
#include "cmds/cmd_dcc.hpp"
#include "cmds/cmd_cap.hpp"
#include "cmds/cmd_whois.hpp"
#include "cmds/cmd_who.hpp"
// #include "cmd_notice.hpp"


void registerAllCommands(CommandRegistry& registry, Server& srv) {
    (void)srv; // 如果暫時沒用到可以忽略警告

    registry.registerCmd("NICK", new CmdNick());
    registry.registerCmd("PASS", new CmdPass());
    registry.registerCmd("USER", new CmdUser());
    registry.registerCmd("PING", new CmdPing());
    registry.registerCmd("JOIN", new CmdJoin());
    registry.registerCmd("PART", new CmdPart());
    registry.registerCmd("PRIVMSG", new CmdPrivmsg());
    registry.registerCmd("KICK",  new CmdKick());
    registry.registerCmd("INVITE",new CmdInvite());
    registry.registerCmd("MODE",  new CmdMode());
    registry.registerCmd("TOPIC", new CmdTopic());
    registry.registerCmd("QUIT", new CmdQuit());
    registry.registerCmd("DCC", new CmdDcc());
    registry.registerCmd("CAP", new CmdCap());
    registry.registerCmd("WHOIS", new CmdWhois());
    registry.registerCmd("WHO", new CmdWho());
}
