# 報告大綱（1 小時版）

## 🎯 報告結構建議

### 📌 第一部分：專案概覽（10 分鐘）

#### 1. 專案目標
```
我們實現了一個符合 IRC 協議的聊天伺服器
- 支援多客戶端連接
- 使用非阻塞 I/O
- 實現基本 IRC 命令
- 符合 ft_irc 評分標準
```

#### 2. 技術選擇
```
- 語言：C++98
- I/O 模型：poll() + 非阻塞 socket
- 設計模式：Command Pattern
- 架構：事件驅動
```

#### 3. 檔案結構
```
src/
  ├── main.cpp           # 入口
  ├── Server.cpp         # 核心邏輯
  ├── Listener.cpp       # 監聽 socket
  ├── Client.cpp         # 客戶端管理
  ├── Channel.cpp        # 頻道管理
  └── cmds/              # 各個命令實現
      ├── cmd_join.cpp
      ├── cmd_privmsg.cpp
      └── ...
```

---

### 📌 第二部分：核心架構（15 分鐘）

#### 1. 整體流程圖

展示 `docs/01_ARCHITECTURE.md` 中的架構圖

**關鍵點：**
- Server 是中心
- Listener 負責新連接
- Client 管理每個連接
- poll() 驅動所有事件

#### 2. 主循環詳解

```cpp
// 偽代碼
while (true) {
    // 1. 等待事件
    poll(&fds);
    
    // 2. 處理事件
    for each fd with events:
        if (新連接) accept()
        if (可讀) recv() + 處理命令
        if (可寫) send()
        if (錯誤) 關閉
    
    // 3. 清理與新增
    關閉待關閉的
    新增新連接的
}
```

**強調：**
- ✅ 只有一個 poll()
- ✅ 所有 I/O 都在 poll() 之後
- ✅ 非阻塞處理

#### 3. 數據流

```
客戶端命令
    ↓
recv() → _inbuf → popLine() → Parser
    ↓
handleIrcMessage() → CommandRegistry
    ↓
具體 Command::execute()
    ↓
sendLine() → _outbuf → send() → 客戶端
```

---

### 📌 第三部分：命令系統（10 分鐘）

#### 1. 設計模式

**Command Pattern 的優點：**
- 易擴展
- 統一接口
- 職責分離

#### 2. 命令註冊

展示 `CommandInit.cpp`:
```cpp
registry.registerCmd("PASS", new CmdPass());
registry.registerCmd("NICK", new CmdNick());
// ...
```

#### 3. 命令執行範例

展示 `CmdPrivmsg::execute()` 的流程

---

### 📌 第四部分：關鍵技術點（15 分鐘）

#### 1. 非阻塞 I/O

**為什麼用非阻塞？**
```
阻塞模式：read() 會等待，阻塞整個伺服器
非阻塞模式：read() 立即返回，用 poll() 等待
```

**我們的實現：**
```cpp
// 所有 socket 都設為 O_NONBLOCK
fcntl(fd, F_SETFL, O_NONBLOCK);

// recv/send 遇到 EAGAIN 直接返回
if (errno == EAGAIN) return;  // 等下次 poll
```

#### 2. 緩衝區管理

**輸入緩衝（_inbuf）：**
- 累積接收的數據
- 按行切割
- 處理部分命令

**輸出緩衝（_outbuf）：**
- 排隊待發送的數據
- 分批發送
- 處理發送阻塞

#### 3. 註冊流程

```
PASS → NICK → USER → 發送 001 歡迎
```

**順序強制：**
- NICK/USER 前必須 PASS
- 未註冊不能用其他命令

#### 4. 頻道廣播

```cpp
broadcastToChannel("#test", message, except_fd) {
    for (每個成員) {
        if (fd != except_fd) {
            member->sendLine(message);
            enableWriteForFd(fd);
        }
    }
}
```

---

### 📌 第五部分：特殊場景處理（10 分鐘）

#### 1. 部分命令（Ctrl+D 測試）

**場景：**
```
PRI[Ctrl+D]VMG[Ctrl+D] #test :hello[Enter]
```

**處理：**
- 累積到 `_inbuf`
- `popLine()` 等待完整行
- 正確解析完整命令

#### 2. 客戶端斷開

**EOF 處理：**
```cpp
if (recv() == 0) {
    // 本輪處理完再關閉
    toCloseIdx.push_back(idx);
}
```

**優雅降級：**
- 完成當前事件處理
- 發送待發消息
- 然後才關閉

#### 3. Ctrl+Z 暫停（重點！）

**場景：**
1. 客戶端 A 按 Ctrl+Z 暫停
2. 其他客戶端發大量消息給 A
3. A 恢復（fg）

**我們的處理：**
```cpp
send() 返回 EAGAIN
    ↓
保留在 _outbuf
    ↓
保持 POLLOUT 監聽
    ↓
下次 poll() 繼續嘗試
    ↓
客戶端恢復後收到所有消息
```

**展示：** 現場演示這個場景

---

### 📌 第六部分：評分重點（5 分鐘）

#### 快速檢查清單

```
✅ 只有一個 poll()
✅ poll() 在所有 I/O 前
✅ fcntl(fd, F_SETFL, O_NONBLOCK)
✅ errno 不觸發重試
✅ 多連接不阻塞
✅ 部分命令正確處理
✅ Ctrl+Z 不 hang
```

#### 可能的問題與回答

**Q: 為什麼空行 Ctrl+D 會關閉連接？**
```
A: TCP EOF 表示客戶端關閉寫通道。
   對 IRC 來說，無法發送命令就無意義。
   這是正確的協議處理。
```

**Q: 如何確保只有一個 poll()？**
```
A: grep -rn "poll(" src/
   只有 Server::run() 中有一個調用
```

**Q: 如何處理大量並發？**
```
A: 所有 fd 在同一個 poll 陣列
   非阻塞 I/O 確保不會互相阻塞
   事件驅動，誰有事件處理誰
```

---

### 📌 第七部分：Demo 演示（5 分鐘）

#### 準備好的終端

**終端 1: 伺服器**
```bash
./ircserv 6667 password123
```

**終端 2: 客戶端 A**
```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test
```

**終端 3: 客戶端 B**
```bash
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
PRIVMSG #test :Hello everyone!
```

**展示重點：**
1. 兩個客戶端都能收到消息
2. 私訊功能
3. Ctrl+Z 測試（如果時間允許）

---

## 🎬 簡報技巧

### 時間分配
```
00:00 - 10:00  專案概覽 + 架構
10:00 - 20:00  核心流程講解
20:00 - 30:00  命令系統
30:00 - 45:00  關鍵技術點
45:00 - 55:00  特殊場景 + Demo
55:00 - 60:00  Q&A
```

### 重點強調

#### 必須提到的點：
1. ✅ **只有一個 poll()**
2. ✅ **非阻塞 I/O**
3. ✅ **事件驅動架構**
4. ✅ **Command Pattern**
5. ✅ **緩衝區管理**

#### 可以展示自信的地方：
1. Ctrl+Z 處理（很多人做不好）
2. 優雅的命令系統
3. 清晰的架構分層
4. 符合所有評分要求

### 預期問題準備

**技術問題：**
- "為什麼用 poll 不用 epoll？" → 題目要求 + 跨平台
- "如何處理大量連接？" → 非阻塞 + 單一 poll
- "記憶體管理如何？" → 每個 Client 是指針，離開時 delete

**實現問題：**
- "如何測試？" → nc + IRC 客戶端 + 真實伺服器對比
- "遇到什麼困難？" → 非阻塞 I/O 理解 + 部分命令處理
- "如何調試？" → 添加 DEBUG 輸出 + gdb

---

## 📝 快速備忘卡

### 核心概念
```
1. 事件驅動：poll() 驅動一切
2. 非阻塞：EAGAIN = 等下次，不是錯誤
3. 緩衝區：累積輸入，排隊輸出
4. 延遲關閉：處理完本輪再關
```

### 關鍵代碼位置
```
- poll() 調用：Server.cpp:122
- 事件處理：Server.cpp:309
- 命令分發：Server.cpp:109
- 非阻塞讀：Client.cpp:31
- 非阻塞寫：Client.cpp:78
```

### 評分要點
```
✅ 1 個 poll
✅ poll 在 I/O 前
✅ fcntl 正確
✅ errno 不重試
✅ 多連接OK
✅ Ctrl+Z OK
```

---

## 💪 給自己的信心

您的實現：
- ✅ 架構清晰
- ✅ 符合所有要求
- ✅ 代碼質量好
- ✅ 有完整的調試信息
- ✅ 特殊場景都處理了

**放輕鬆，您準備得很好！** 🎉

