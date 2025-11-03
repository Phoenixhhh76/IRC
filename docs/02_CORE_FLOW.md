# 核心流程詳解

## 🚀 伺服器啟動流程

### main.cpp
```cpp
int main(int argc, char** argv) {
    // 1. 解析參數
    int port = atoi(argv[1]);
    string password = argv[2];
    
    // 2. 創建伺服器
    Server server(port, password);
    
    // 3. 運行主循環
    server.run();  // ← 永不返回，直到 Ctrl+C
}
```

### Server 建構子
```cpp
Server::Server(int port, const string& password)
    : _listener(port),  // ← 創建監聽 socket
      _password(password) {
    
    // 1. 將 listener fd 加入 poll 陣列
    pollfd p;
    p.fd = _listener.getFd();
    p.events = POLLIN;  // ← 只監聽可讀（新連接）
    _pfds.push_back(p);
    
    // 2. 註冊所有命令
    initCommands();
}
```

## 🔄 主事件循環 (Server::run)

```cpp
void Server::run() {
    for (;;) {  // ← 無限循環
        // ========== 1. 等待事件 ==========
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        // -1 = 無限等待，直到有事件
        
        if (ready < 0) {
            if (errno == EINTR) continue;  // 信號打斷，重試
            throw runtime_error("poll failed");
        }
        
        // ========== 2. 處理所有事件 ==========
        vector<int> toAddFds;      // 新連接
        vector<size_t> toCloseIdx; // 要關閉的索引
        
        for (size_t i = 0; i < _pfds.size(); ++i) {
            if (_pfds[i].revents) {  // ← 有事件發生
                processPollEvent(i, toAddFds, toCloseIdx);
            }
        }
        
        // ========== 3. 清理與新增 ==========
        // 先關閉（倒序，避免索引錯位）
        for (size_t k = 0; k < toCloseIdx.size(); ++k) {
            size_t idx = toCloseIdx[toCloseIdx.size() - 1 - k];
            closeClient(idx);
        }
        
        // 再新增
        for (size_t j = 0; j < toAddFds.size(); ++j) {
            addNewClient(toAddFds[j]);
        }
    }
}
```

## 📥 事件處理 (processPollEvent)

```cpp
void Server::processPollEvent(size_t idx, ...) {
    pollfd cur = _pfds[idx];
    
    // ========== A. 新連接 (Listener) ==========
    if ((cur.revents & POLLIN) && cur.fd == _listener.getFd()) {
        vector<pair<int, string>> news = acceptClients(_listener.getFd());
        // acceptClients() 會循環 accept() 直到 EAGAIN
        
        for (每個新 fd) {
            // 創建 Client 對象
            Client* newClient = new Client(fd);
            _clients[fd] = newClient;
            
            // 加入 poll 陣列
            pollfd np;
            np.fd = fd;
            np.events = POLLIN;  // ← 監聽客戶端輸入
            _pfds.push_back(np);
        }
    }
    
    // ========== B. 客戶端可讀 (POLLIN) ==========
    else if (cur.revents & POLLIN) {
        Client* cl = _clients[fd];
        
        // 1. 讀取數據
        if (!cl->readFromSocket()) {  // ← recv()
            toCloseIdx.push_back(idx);  // EOF 或錯誤
            return;
        }
        
        // 2. 逐行處理命令
        string line;
        while (cl->popLine(line)) {  // ← 從緩衝區切出完整行
            IrcMessage m = parseLine(line);
            handleIrcMessage(*cl, m);  // ← 處理命令
        }
    }
    
    // ========== C. 客戶端可寫 (POLLOUT) ==========
    else if (cur.revents & POLLOUT) {
        Client* cl = _clients[fd];
        
        cl->flushOutbuf();  // ← send()
        
        // 如果送完了，關閉 POLLOUT（避免 busy loop）
        if (cl->isOutbufEmpty()) {
            _pfds[idx].events &= ~POLLOUT;
        }
    }
    
    // ========== D. 錯誤/關閉 ==========
    if (cur.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        toCloseIdx.push_back(idx);
    }
}
```

## 📤 非阻塞讀寫

### 讀取 (Client::readFromSocket)
```cpp
bool Client::readFromSocket() {
    char buf[4096];
    for (;;) {  // ← 循環讀，直到沒數據
        ssize_t n = recv(_fd, buf, sizeof(buf), 0);
        
        if (n > 0) {
            _inbuf.append(buf, n);  // ← 累積到緩衝區
            continue;  // ← 繼續讀
        }
        
        if (n == 0) return false;  // ← EOF，需要關閉
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;  // ← 本輪讀完，等下次 poll
        }
        
        return false;  // ← 錯誤
    }
}
```

### 寫入 (Client::flushOutbuf)
```cpp
void Client::flushOutbuf() {
    while (!_outbuf.empty()) {  // ← 循環發送
        ssize_t n = send(_fd, _outbuf.data(), _outbuf.size(), 0);
        
        if (n > 0) {
            _outbuf.erase(0, n);  // ← 移除已發送部分
            continue;  // ← 繼續發送
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;  // ← 暫時發不出去，下次 poll 再試
        }
        
        throw runtime_error("send error");
    }
}
```

## 🔐 註冊流程

```
客戶端連接
    ↓
PASS password123  ← 必須第一個
    ↓
NICK alice
    ↓
USER alice 0 * :Alice Smith
    ↓
Server 檢查：hasPass && hasNick && hasUser
    ↓
發送 RPL_WELCOME (001)
    ↓
registered = true
    ↓
可以使用其他命令
```

## ⏱️ 時序圖：發送消息

```
時間  客戶端 A         poll()         Server          客戶端 B
──────────────────────────────────────────────────────────
T0   PRIVMSG B :hi
     ─────────────→  POLLIN         recv()
                     A 的事件        解析命令
                                    找到 B
                                    B->sendLine(...)
                                    B._outbuf = "..."
                                    enableWriteForFd(B)
                                    _pfds[B].events |= POLLOUT
                                                        
T1                   POLLOUT        send()         ←────
                     B 的事件       B._outbuf      收到消息
                                    發送成功
                                    _pfds[B].events &= ~POLLOUT
```

## 🎯 關鍵點

1. **單一 poll()**：所有等待都在一個 poll 調用
2. **事件驅動**：只在 poll 返回後才做 I/O
3. **非阻塞**：EAGAIN 不是錯誤，而是"等下次 poll"
4. **緩衝區**：_inbuf 累積、_outbuf 排隊
5. **延遲關閉**：本輪處理完才關閉，避免消息丟失

