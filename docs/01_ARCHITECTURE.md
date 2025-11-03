# IRC 伺服器架構總覽

## 🏗️ 整體架構

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│  - 解析參數 (port, password)                             │
│  - 創建 Server 實例                                      │
│  - 調用 server.run()                                    │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    Server 類別                           │
│  核心成員:                                               │
│  - Listener _listener          (監聽新連接)              │
│  - vector<pollfd> _pfds        (poll 監聽所有 fd)        │
│  - map<int, Client*> _clients  (管理所有客戶端)          │
│  - CommandRegistry _cmds       (命令註冊系統)            │
│  - map<string, Channel> _channels (所有頻道)            │
└─────────────────────────────────────────────────────────┘
                            ↓
        ┌──────────────────┼──────────────────┐
        ↓                  ↓                  ↓
   ┌─────────┐      ┌──────────┐      ┌──────────┐
   │Listener │      │ Client   │      │ Channel  │
   │(socket) │      │(每個連接)│      │(頻道管理)│
   └─────────┘      └──────────┘      └──────────┘
```

## 📂 核心文件結構

### 1. 網路層
- `Listener.cpp/hpp` - 監聽 socket，接受新連接
- `acceptClients.cpp/hpp` - 處理 accept() 並設置非阻塞
- `Client.cpp/hpp` - 單個客戶端的讀寫緩衝區管理

### 2. 事件循環
- `Server.cpp` 中的 `run()` - 主 poll 循環
- `processPollEvent()` - 處理每個 fd 的事件

### 3. 命令系統
- `CommandRegistry.cpp/hpp` - 命令註冊器
- `CommandInit.cpp` - 註冊所有命令
- `cmds/cmd_*.cpp` - 各個命令的實現

### 4. 協議層
- `Parser.cpp/hpp` - 解析 IRC 消息
- `Replies.hpp` - IRC 數字回應碼

### 5. 邏輯層
- `Channel.cpp/hpp` - 頻道管理、模式、成員
- `Server.cpp` - 業務邏輯協調

## 🔑 關鍵設計模式

### 1. 非阻塞 I/O
- 所有 socket 設為 `O_NONBLOCK`
- 使用 `poll()` 監聽事件
- `recv()`/`send()` 返回 `EAGAIN` 時不重試，等待下次 poll

### 2. 命令模式 (Command Pattern)
- 每個命令是一個獨立的類
- 統一接口：`execute(Server&, Client&, IrcMessage&)`
- 在 `CommandRegistry` 中註冊

### 3. 事件驅動
- `poll()` 驅動整個系統
- 事件分類：POLLIN (可讀)、POLLOUT (可寫)、POLLERR (錯誤)

## 📊 數據流

```
客戶端發送命令
    ↓
poll() 返回 POLLIN
    ↓
recv() 讀取到 _inbuf
    ↓
popLine() 切出完整行
    ↓
Parser::parseLine() 解析
    ↓
Server::handleIrcMessage() 分發
    ↓
CommandRegistry::dispatch() 路由
    ↓
具體 Cmd 類的 execute()
    ↓
Client::sendLine() 放入 _outbuf
    ↓
enableWriteForFd() 啟用 POLLOUT
    ↓
poll() 返回 POLLOUT
    ↓
send() 發送給客戶端
```

## 🎯 評分關鍵點

✅ **只有一個 poll()**
- 位置：`Server::run()` 的主循環

✅ **poll() 在所有 I/O 之前**
- accept/recv/send 都在 poll() 返回後才調用

✅ **fcntl() 格式正確**
- 都使用 `fcntl(fd, F_SETFL, O_NONBLOCK)`

✅ **非阻塞處理**
- EAGAIN 不重試，等待下次 poll

✅ **多連接處理**
- 所有 fd 都在同一個 poll 陣列中

