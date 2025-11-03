# 評分關鍵點速查

## ✅ Basic Checks（必須通過，否則 0 分）

### 1. Makefile 與編譯
```bash
# 檢查項目
- [✅] 有 Makefile
- [✅] 編譯無錯誤無警告
- [✅] 使用 C++98
- [✅] 可執行文件名為 ircserv
- [✅] 編譯選項：-Wall -Wextra -Werror -std=c++98
```

**展示：**
```bash
make clean
make
# 無警告，無錯誤
```

### 2. poll() 數量檢查

**要求：** 只能有**一個** poll() 調用

**我們的實現：**
```cpp
// src/Server.cpp:122
void Server::run() {
    for (;;) {
        int ready = ::poll(&_pfds[0], _pfds.size(), -1);  // ← 唯一的 poll()
        // ...
    }
}
```

**證明：**
```bash
grep -rn "poll(" src/ include/
# 只會找到一處 poll() 調用
```

### 3. poll() 調用時機

**要求：** poll() 必須在每次 accept/read/recv/write/send 之前調用

**我們的流程：**
```
poll() 返回
    ↓
檢查 POLLIN → 才調用 accept()/recv()
檢查 POLLOUT → 才調用 send()
```

**關鍵代碼位置：**
- `Server.cpp:122` - poll() 調用
- `Server.cpp:313` - accept() 在 POLLIN 之後
- `Server.cpp:344` - recv() 在 POLLIN 之後  
- `Server.cpp:378` - send() 在 POLLOUT 之後

### 4. fcntl() 使用檢查

**要求：** 所有 fcntl() 必須是 `fcntl(fd, F_SETFL, O_NONBLOCK)`

**我們的實現：**
```cpp
// src/Listener.cpp:71
fcntl(fd, F_SETFL, O_NONBLOCK)

// src/acceptClients.cpp:14
fcntl(fd, F_SETFL, O_NONBLOCK)

// src/bonus/Dcc.cpp:74 (已修復)
fcntl(s, F_SETFL, O_NONBLOCK)
```

**證明：**
```bash
grep -rn "fcntl" src/
# 所有都是 F_SETFL, O_NONBLOCK 格式
```

### 5. errno 使用檢查

**要求：** errno 不應該用來觸發重試（如 `errno == EAGAIN` 後再次 read）

**我們的實現：**
```cpp
// Client::readFromSocket()
if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return true;  // ← 直接返回，等待下次 poll()，不重試！
}

// Client::flushOutbuf()
if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return;  // ← 直接返回，等待下次 poll()，不重試！
}
```

## 🌐 Networking（網路功能）

### 1. 伺服器啟動
```bash
./ircserv 6667 password123
# 監聽所有介面 (0.0.0.0:6667)
```

**證明：**
```bash
netstat -an | grep 6667
# 應該看到 0.0.0.0:6667 LISTEN
```

### 2. nc 連接測試
```bash
nc localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
# 應該收到歡迎消息
```

### 3. IRC 客戶端測試
```bash
# 使用 irssi 或 hexchat
/connect localhost 6667 password123
/nick testuser
```

### 4. 多連接測試

**展示：** 同時開啟 3 個終端
- 終端 A: nc 連接
- 終端 B: IRC 客戶端連接
- 終端 C: 另一個 nc 連接

**證明非阻塞：** 所有連接都能同時工作

### 5. 頻道消息廣播
```bash
# 客戶端 A
JOIN #test
PRIVMSG #test :Hello from A

# 客戶端 B（同一頻道）
# 應該收到 A 的消息
```

## 🔧 Network Specials（特殊場景）

### 1. 部分命令測試

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
# 測試分段命令（行中 Ctrl+D）
PRI[Ctrl+D]VMG[Ctrl+D] #test :hello[Enter]
# ✅ 應該成功解析完整的 PRIVMSG 命令
```

**原理：** 
- 累積到 `_inbuf`
- `popLine()` 只在收到完整行時才返回

### 2. 意外斷開客戶端

```bash
# 終端 A: nc 連接
# 終端 B: kill -9 <A的進程ID>
# 或在 A 終端按 Ctrl+C

# 伺服器應該：
- [✅] 正確關閉該客戶端
- [✅] 其他連接繼續工作
- [✅] 不會崩潰
```

### 3. 半命令斷開

```bash
nc localhost 6667
PASS password123
NICK alice
JOIN #t[Ctrl+C]  ← 半個命令後中斷

# 伺服器應該：
- [✅] 不會進入奇怪狀態
- [✅] 不會阻塞
- [✅] 正確清理資源
```

### 4. Ctrl+Z 測試（重要！）

```bash
# 終端 A
nc localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test
[Ctrl+Z]  ← 暫停

# 終端 B
nc localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
# 發送大量消息給 alice
PRIVMSG alice :msg1
PRIVMSG alice :msg2
...（很多消息）

# 終端 A
fg  ← 恢復

# 檢查：
- [✅] alice 收到所有消息
- [✅] 伺服器沒有 hang
- [✅] 沒有記憶體洩漏
```

**原理：**
- `send()` 返回 `EAGAIN` 時保留在 `_outbuf`
- 保持 `POLLOUT` 監聽
- 客戶端恢復後繼續發送

## 📝 Client Commands

### 基本命令測試

```bash
# 1. 註冊流程
PASS password123
NICK alice
USER alice 0 * :Alice Smith
# ✅ 收到 001 歡迎消息

# 2. PRIVMSG 測試
PRIVMSG bob :Hello Bob        # 私訊
PRIVMSG #test :Hello channel  # 頻道消息

# 3. JOIN/PART
JOIN #test
PART #test :Goodbye

# 4. TOPIC
TOPIC #test :New topic

# 5. MODE
MODE #test +t
MODE #test +l 10
```

## 🎯 展示建議順序

### 第一部分：基本檢查（5 分鐘）
1. 展示 Makefile 編譯
2. 證明只有一個 poll()
3. 展示 fcntl() 使用正確

### 第二部分：網路功能（10 分鐘）
1. 啟動伺服器
2. nc 連接測試
3. 多連接同時運作
4. 頻道消息廣播

### 第三部分：特殊場景（10 分鐘）
1. 部分命令（行中 Ctrl+D）
2. 客戶端斷開
3. **Ctrl+Z 暫停測試（重點！）**

### 第四部分：程式碼講解（15 分鐘）
1. 架構圖
2. 主循環流程
3. 命令系統
4. 非阻塞 I/O 處理

## 📊 評分檢查表

```
基本檢查 (必須全過)
□ Makefile 正確
□ 只有一個 poll()
□ poll() 在 I/O 前調用
□ fcntl() 格式正確
□ errno 不觸發重試

網路功能
□ 伺服器啟動並監聽
□ nc 可以連接
□ IRC 客戶端可以連接
□ 多連接同時工作
□ 頻道消息正確廣播

特殊場景
□ 部分命令正確處理
□ 客戶端斷開正確清理
□ Ctrl+Z 不阻塞伺服器
□ 恢復後消息正常

命令功能
□ PASS/NICK/USER 註冊
□ PRIVMSG 私訊和頻道
□ JOIN/PART 正確
□ 其他命令實現
```

## 🔑 關鍵話術

**當評審問「為什麼...」時：**

- **Q: 為什麼空行 Ctrl+D 會關閉？**
  - A: "這是 TCP EOF，表示客戶端關閉寫通道。對 IRC 來說，無法發送命令後保持連接沒意義。"

- **Q: 為什麼只有一個 poll()？**
  - A: "這是要求。所有 fd（listener + 所有客戶端）都在同一個 poll 陣列中監聽。"

- **Q: 如何處理非阻塞 I/O？**
  - A: "EAGAIN 不是錯誤，而是'暫時沒數據/無法寫入'，我們直接返回，等待下次 poll() 通知。"

