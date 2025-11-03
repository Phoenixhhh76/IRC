# 命令系統詳解

## 🎯 設計模式：Command Pattern

每個 IRC 命令都是一個獨立的類，統一接口，集中註冊。

## 📋 系統架構

```
IrcMessage (解析結果)
    ↓
Server::handleIrcMessage() (檢查註冊狀態)
    ↓
CommandRegistry::dispatch() (路由到正確命令)
    ↓
具體 Command 類的 execute() (執行邏輯)
    ↓
Client::sendLine() (回應客戶端)
```

## 🔧 核心組件

### 1. Command 基類 (Command.hpp)

```cpp
class Command {
public:
    virtual ~Command() {}
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m) = 0;
};
```

所有命令都繼承這個接口。

### 2. CommandRegistry (命令註冊器)

```cpp
class CommandRegistry {
private:
    map<string, Command*> _cmds;  // 命令名 -> 命令對象

public:
    void registerCmd(const string& name, Command* cmd) {
        _cmds[name] = cmd;
    }
    
    bool dispatch(const string& cmd, Server& srv, Client& cl, const IrcMessage& m) {
        map<string, Command*>::iterator it = _cmds.find(cmd);
        if (it == _cmds.end()) return false;  // 未知命令
        
        it->second->execute(srv, cl, m);  // ← 調用具體命令
        return true;
    }
};
```

### 3. CommandInit (命令註冊)

```cpp
void registerAllCommands(CommandRegistry& registry, Server& srv) {
    registry.registerCmd("PASS", new CmdPass());
    registry.registerCmd("NICK", new CmdNick());
    registry.registerCmd("USER", new CmdUser());
    registry.registerCmd("JOIN", new CmdJoin());
    registry.registerCmd("PRIVMSG", new CmdPrivmsg());
    registry.registerCmd("QUIT", new CmdQuit());
    // ... 其他命令
}
```

## 📝 命令實現範例

### NICK 命令

```cpp
class CmdNick : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m) {
        // 1. 參數檢查
        if (m.params.empty()) {
            cl.sendLine(ERR_NONICKNAMEGIVEN(srv.serverName()));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        const string& nick = m.params[0];
        
        // 2. 合法性檢查
        if (!isValidNick(nick)) {
            cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), nick));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        // 3. 重複檢查
        if (srv.isNickInUse(nick)) {
            cl.sendLine(ERR_NICKNAMEINUSE(srv.serverName(), nick));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        // 4. 更新暱稱
        srv.takeNick(cl, nick);
        
        // 5. 廣播給所有相關頻道
        const set<string>& channels = cl.channels();
        for (set<string>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
            srv.broadcastToChannel(*it, oldPrefix + " NICK :" + nick, -1);
        }
        
        // 6. 如果是註冊過程的一部分，發送歡迎消息
        if (cl.tryFinishRegister()) {
            srv.sendWelcome(cl);
        }
    }
};
```

### PRIVMSG 命令

```cpp
class CmdPrivmsg : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m) {
        // 1. 參數檢查
        if (m.params.size() < 2) {
            cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "PRIVMSG"));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        const string& target = m.params[0];
        const string& text = m.params[1];
        string line = cl.getFullPrefix() + " PRIVMSG " + target + " :" + text;
        
        // 2. 判斷目標類型
        if (target[0] == '#') {
            // 發送到頻道
            if (!srv.isChannelMember(target, cl.fd())) {
                cl.sendLine(ERR_CANNOTSENDTOCHAN(srv.serverName(), target));
                srv.enableWriteForFd(cl.fd());
                return;
            }
            srv.broadcastToChannel(target, line, cl.fd());
        } else {
            // 發送給用戶
            srv.sendToNick(target, line);
        }
    }
};
```

## 🔐 權限檢查 (Server::handleIrcMessage)

```cpp
void Server::handleIrcMessage(Client& cl, const IrcMessage& m) {
    const string& cmd = m.command;
    
    // ========== 未註冊客戶端的限制 ==========
    if (!cl.registered()) {
        // 必須先 PASS
        if (!cl.hasPass() && (cmd == "NICK" || cmd == "USER")) {
            cl.sendLine(ERR_PASSWDMISMATCH(_servername));
            enableWriteForFd(cl.fd());
            return;
        }
        
        // 只允許特定命令
        if (cmd != "PASS" && cmd != "NICK" && cmd != "USER" && 
            cmd != "PING" && cmd != "CAP" && cmd != "QUIT") {
            cl.sendLine(ERR_NOTREGISTERED(_servername));
            enableWriteForFd(cl.fd());
            return;
        }
    }
    
    // ========== 分發命令 ==========
    if (!_cmds.dispatch(cmd, *this, cl, m)) {
        // 未知命令（可選擇發送錯誤）
    }
}
```

## 📊 命令流程圖

```
客戶端發送: "PRIVMSG #test :Hello"
    ↓
Parser::parseLine()
    ↓
IrcMessage {
    command: "PRIVMSG"
    params: ["#test", "Hello"]
}
    ↓
Server::handleIrcMessage()
    ├─ 檢查是否註冊？ ✓
    └─ _cmds.dispatch("PRIVMSG", ...)
           ↓
       CmdPrivmsg::execute()
           ├─ 檢查參數 ✓
           ├─ 判斷目標是頻道 ✓
           ├─ 檢查是否在頻道內 ✓
           └─ broadcastToChannel("#test", ...)
                  ↓
              遍歷頻道成員
                  ├─ member1->sendLine(...)
                  ├─ member2->sendLine(...)
                  └─ member3->sendLine(...)
                         ↓
                  enableWriteForFd() for each
                         ↓
                  下次 poll() 時 POLLOUT 觸發
                         ↓
                  send() 發送給客戶端
```

## 🎯 重點命令列表

| 命令 | 功能 | 是否需要註冊 |
|------|------|------------|
| **PASS** | 密碼驗證 | ❌ |
| **NICK** | 設置暱稱 | ❌ |
| **USER** | 設置用戶信息 | ❌ |
| **JOIN** | 加入頻道 | ✅ |
| **PART** | 離開頻道 | ✅ |
| **PRIVMSG** | 發送消息 | ✅ |
| **QUIT** | 斷開連接 | ❌ |
| **PING** | 心跳 | ❌ |
| **KICK** | 踢出用戶 | ✅ |
| **MODE** | 設置模式 | ✅ |
| **TOPIC** | 設置主題 | ✅ |

## 🔑 優點

1. **易擴展**：新增命令只需創建新類並註冊
2. **統一接口**：所有命令都用相同的 execute() 接口
3. **集中管理**：CommandRegistry 統一路由
4. **職責分離**：每個命令只負責自己的邏輯
5. **易測試**：可以單獨測試每個命令類

## 💡 新增命令步驟

1. 創建 `cmd_xxx.hpp` 和 `cmd_xxx.cpp`
2. 繼承 `Command` 基類
3. 實現 `execute()` 方法
4. 在 `CommandInit.cpp` 中註冊
5. 在 `Makefile` 中添加源文件

