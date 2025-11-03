# 控制鍵詳解：Ctrl+D vs Ctrl+C vs Ctrl+Z

## 🎯 快速對比

| 控制鍵 | 本質 | 作用 | 對 nc 的影響 | 對伺服器的影響 |
|--------|------|------|------------|--------------|
| **Ctrl+D** | EOF 字符 | 結束輸入 | 關閉寫端（發送 FIN） | recv() 返回 0 |
| **Ctrl+C** | SIGINT 信號 | 中斷程序 | **立即殺死 nc** | recv() 可能返回 0 或錯誤 |
| **Ctrl+Z** | SIGTSTP 信號 | 暫停程序 | **暫停 nc** | 發送可能返回 EAGAIN |

---

## 🔍 Ctrl+D - EOF（文件結束）

### 本質
- **End of File** 字符
- ASCII 值：4（控制字符）
- **不是**信號，而是輸入字符

### 行為

#### 行中有內容時
```bash
hello[Ctrl+D]
```

**發生：**
```
終端：把 "hello" 發送給 nc
nc：發送 "hello" 到 socket
伺服器：recv() 收到 "hello"
nc：繼續運行 ✅
```

#### 空行時
```bash
[空行 Ctrl+D]
```

**發生：**
```
終端：發送 EOF 給 nc
nc：「stdin 結束了」
nc：shutdown(socket, SHUT_WR)
nc：發送 TCP FIN 到伺服器
伺服器：recv() 返回 0（EOF）
nc：macOS 退出 / Ubuntu 可能不退出
```

### 特點
- ✅ **優雅關閉**：給雙方時間處理
- ✅ **可能半關閉**：只關閉寫端，讀端仍開啟
- ✅ **有清理時間**：伺服器可以發送最後的消息
- ⚠️ **可能不立即退出**（取決於 nc 版本）

---

## ⚡ Ctrl+C - SIGINT（中斷信號）

### 本質
- **Signal Interrupt** 信號
- 信號編號：2
- 發送給前台進程

### 行為

```bash
nc localhost 6667
PASS password123
NICK alice
[Ctrl+C]  ← 任何時候按
```

**發生：**
```
1. 作業系統發送 SIGINT 給 nc
   ↓
2. nc 收到信號
   ↓
3. nc 的預設處理：立即終止程序
   ↓
4. nc 程序被殺死（類似 kill）
   ↓
5. 作業系統關閉 nc 的所有 fd
   ↓
6. socket 被關閉（粗暴關閉）
   ↓
7. 伺服器端：
   - 可能 recv() 返回 0（對端關閉）
   - 可能 recv() 返回錯誤（連接重置）
   - 可能看到 POLLHUP 事件
```

### 特點
- ⚡ **立即終止**：不管在做什麼
- ❌ **不優雅**：沒有清理時間
- ❌ **強制關閉**：可能丟失未發送的數據
- ✅ **總是有效**：在任何狀態都能退出

---

## ⏸️ Ctrl+Z - SIGTSTP（暫停信號）

### 本質
- **Signal Terminal Stop** 信號
- 信號編號：18（Unix）/ 20（Linux）
- 暫停進程，不終止

### 行為

```bash
nc localhost 6667
PASS password123
NICK alice
[Ctrl+Z]
```

**發生：**
```
1. 作業系統發送 SIGTSTP 給 nc
   ↓
2. nc 進程暫停（狀態：Stopped）
   ↓
3. nc 不再執行任何代碼
   ↓
4. socket 仍然開啟 ✅
   ↓
5. 伺服器端：
   - send() 可能返回 EAGAIN（緩衝區滿）
   - 連接仍然存在
   ↓
6. 用戶執行 fg（foreground）
   ↓
7. nc 恢復運行
   ↓
8. 從 socket 讀取累積的數據
```

### 特點
- ⏸️ **可恢復**：fg 恢復，bg 後台運行
- ✅ **連接保持**：socket 不關閉
- ✅ **數據不丟失**：累積在緩衝區
- 🧪 **測試用**：測試伺服器的非阻塞處理

---

## 📊 詳細對比表

### 對終端輸入的影響

| 控制鍵 | 輸入處理 | 後續能否輸入 |
|--------|---------|------------|
| **Ctrl+D** | 發送當前緩衝區 | ✅ 可以（如果行中有內容）<br>❌ 不能（如果空行） |
| **Ctrl+C** | 丟棄當前行 | ❌ 不能（程序終止） |
| **Ctrl+Z** | 保留當前行 | ✅ 可以（fg 恢復後） |

### 對程序的影響

| 控制鍵 | 程序狀態 | socket 狀態 | 可恢復性 |
|--------|---------|------------|---------|
| **Ctrl+D** | 繼續運行 | 寫端關閉 | ❌ 寫端無法恢復 |
| **Ctrl+C** | **終止** | 關閉 | ❌ 程序終止 |
| **Ctrl+Z** | **暫停** | 保持開啟 | ✅ fg/bg 恢復 |

### 對伺服器的影響

| 控制鍵 | 伺服器 recv() | 伺服器處理 | 連接清理 |
|--------|--------------|----------|---------|
| **Ctrl+D** | 返回 0 (EOF) | 標記關閉，延遲處理 | 優雅清理 |
| **Ctrl+C** | 返回 0 或錯誤 | 立即清理 | 可能粗暴 |
| **Ctrl+Z** | EAGAIN（暫時無數據） | 繼續運行，不清理 | 無清理 |

---

## 🎬 實際測試對比

### 測試環境準備

```bash
# 終端 1: 伺服器
./ircserv 6667 password123

# 終端 2: 客戶端 A
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test

# 終端 3: 客戶端 B
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
```

### 測試 1：Ctrl+D（EOF）

```bash
# 終端 A
[空行 Ctrl+D]
```

**伺服器輸出：**
```
[DEBUG] Client fd=4 (alice) recv EOF, closing
[DEBUG] Marking client idx=1 fd=4 for close
[DEBUG] closeClient called for idx=1
[DEBUG] Closing client fd=4
```

**終端 A 輸出：**
```
Connection closed by foreign host.  (macOS)
或
（仍在運行，等待）  (Ubuntu)
```

**終端 B 測試：**
```bash
PRIVMSG alice :test
# → :ft_irc 401 alice :No such nick  ✅ alice 已不存在
```

### 測試 2：Ctrl+C（中斷）

```bash
# 終端 A（新連接）
nc -C localhost 6667
PASS password123
NICK charlie
USER charlie 0 * :Charlie
JOIN #test
[Ctrl+C]  ← 立即按
```

**伺服器輸出：**
```
[DEBUG] Client fd=5 (charlie) recv EOF, closing
或
[DEBUG] Client fd=5 recv error: Connection reset by peer
[DEBUG] closeClient called for idx=2
```

**終端 A 輸出：**
```
^C  ← 顯示 Ctrl+C
（nc 程序立即終止）✅ 所有平台都一樣
```

**差異：**
- ⚡ **立即退出**（不像 Ctrl+D 在 Ubuntu 上可能不退出）
- ❌ **可能更粗暴**（連接可能顯示為 reset 而不是正常關閉）

### 測試 3：Ctrl+Z（暫停）

```bash
# 終端 A（新連接）
nc -C localhost 6667
PASS password123
NICK david
USER david 0 * :David
JOIN #test
[Ctrl+Z]
```

**終端 A 輸出：**
```
[1]+  Stopped                 nc -C localhost 6667
```

**伺服器輸出：**
```
（沒有任何錯誤或 EOF 消息）
（david 仍然在線）
```

**終端 B 測試：**
```bash
PRIVMSG david :Are you there?
# 伺服器會嘗試發送給 david
# send() 可能返回 EAGAIN（david 的緩衝區滿了）
# 消息保留在 _outbuf 中
```

**終端 A 恢復：**
```bash
fg
# david 的 nc 恢復
# 收到所有累積的消息！✅
```

---

## 📚 使用場景

### Ctrl+D - 正常退出測試

```bash
# 用途：
- 測試 EOF 處理
- 測試部分命令（行中 Ctrl+D）
- 正常結束輸入

# 適合：
- 互動式測試
- 驗證伺服器 EOF 處理
- 管道輸入結束
```

### Ctrl+C - 強制終止

```bash
# 用途：
- 立即退出 nc
- 測試客戶端意外斷開
- 終止卡住的程序

# 適合：
- nc 不退出時強制關閉
- 測試伺服器的異常處理
- 緊急停止
```

### Ctrl+Z - 暫停測試

```bash
# 用途：
- 測試非阻塞 I/O
- 測試緩衝區管理
- 測試伺服器不會 hang

# 適合：
- 評分要求的 Ctrl+Z 測試
- 驗證消息累積
- 驗證無記憶體洩漏
```

---

## 🎯 評分測試場景

### 場景 1：部分命令（Ctrl+D）

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
PRI[Ctrl+D]VMG[Ctrl+D] #test :msg[Enter]  ← 行中 Ctrl+D
# ✅ 測試部分命令處理
```

### 場景 2：EOF 斷開（Ctrl+D）

```bash
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
[空行 Ctrl+D]  ← 空行 Ctrl+D
# ✅ 測試優雅關閉
```

### 場景 3：意外斷開（Ctrl+C）

```bash
nc -C localhost 6667
PASS password123
NICK charlie
USER charlie 0 * :Charlie
[Ctrl+C]  ← 強制終止
# ✅ 測試異常斷開處理
```

### 場景 4：暫停恢復（Ctrl+Z）

```bash
# 終端 A
nc -C localhost 6667
PASS password123
NICK david
USER david 0 * :David
JOIN #test
[Ctrl+Z]  ← 暫停

# 終端 B：發送大量消息
PRIVMSG david :msg1
PRIVMSG david :msg2
...

# 終端 A：恢復
fg
# 收到所有消息 ✅
# ✅ 測試非阻塞 I/O 和緩衝區
```

---

## 🔧 技術細節

### Ctrl+D 的處理流程

```
用戶層
    ↓
按 Ctrl+D
    ↓
終端（TTY）層
    ↓
檢查當前行緩衝區
    ↓
   ┌─────────────┴─────────────┐
   │                           │
有內容                      沒內容
   ↓                           ↓
發送緩衝區內容          發送 EOF 給程序
   ↓                           ↓
nc 收到資料              nc 收到 EOF
   ↓                           ↓
發送到 socket             shutdown(SHUT_WR)
   ↓                           ↓
繼續運行                    （可能）關閉
```

### Ctrl+C 的處理流程

```
用戶層
    ↓
按 Ctrl+C
    ↓
終端發送 SIGINT
    ↓
作業系統
    ↓
發送信號給前台進程組
    ↓
nc 收到 SIGINT
    ↓
nc 的預設處理：終止程序
    ↓
作業系統清理 nc
    ↓
關閉所有 fd（包括 socket）
    ↓
伺服器端可能看到：
- recv() 返回 0
- recv() 返回錯誤
- POLLHUP 事件
```

### Ctrl+Z 的處理流程

```
用戶層
    ↓
按 Ctrl+Z
    ↓
終端發送 SIGTSTP
    ↓
作業系統
    ↓
發送信號給前台進程
    ↓
nc 收到 SIGTSTP
    ↓
nc 的預設處理：暫停（不終止）
    ↓
nc 進程狀態：Stopped (T)
    ↓
socket 仍然開啟 ✅
fd 仍然有效 ✅
    ↓
伺服器端：
- send() 可能 EAGAIN（nc 不讀了）
- 連接仍然活躍
    ↓
用戶執行 fg
    ↓
nc 恢復運行
    ↓
從 socket 讀取累積的數據
```

---

## 💡 伺服器端的視角

### recv() 的不同結果

```cpp
// Ctrl+D（空行）
ssize_t n = recv(fd, buf, size, 0);
// n = 0  ← EOF
// errno = 0

// Ctrl+C
ssize_t n = recv(fd, buf, size, 0);
// n = 0  ← 連接關閉
// 或 n = -1, errno = ECONNRESET  ← 連接重置

// Ctrl+Z
ssize_t n = recv(fd, buf, size, 0);
// n = -1, errno = EAGAIN  ← 暫時沒數據
// （之後恢復會有數據）
```

### poll() 的不同事件

```cpp
// Ctrl+D（空行）
if (revents & POLLIN) {
    recv() 返回 0
}

// Ctrl+C
if (revents & POLLHUP) {  // 可能
    // 對端掛斷
}
if (revents & POLLIN) {  // 或
    recv() 返回 0 或錯誤
}

// Ctrl+Z
if (revents & POLLOUT) {
    send() 返回 EAGAIN  // 可能（緩衝區滿）
}
// 沒有 POLLIN（nc 暫停，不發送數據）
```

---

## 🧪 實際測試驗證

### 測試腳本

```bash
#!/bin/bash
# test_ctrl_keys.sh

echo "啟動伺服器（在另一個終端）：./ircserv 6667 password123"
echo ""

echo "=== 測試 1: Ctrl+D (EOF) ==="
echo "1. nc -C localhost 6667"
echo "2. 完成註冊（PASS/NICK/USER）"
echo "3. 按空行 Ctrl+D"
echo "4. 觀察伺服器輸出：應該看到 'recv EOF'"
read -p "按 Enter 繼續..."

echo ""
echo "=== 測試 2: Ctrl+C (中斷) ==="
echo "1. nc -C localhost 6667"
echo "2. 完成註冊"
echo "3. 按 Ctrl+C"
echo "4. 觀察：立即退出（所有平台）"
read -p "按 Enter 繼續..."

echo ""
echo "=== 測試 3: Ctrl+Z (暫停) ==="
echo "1. 客戶端 A: nc -C localhost 6667，完成註冊"
echo "2. 客戶端 A: 按 Ctrl+Z"
echo "3. 客戶端 B: 發送大量消息給 A"
echo "4. 客戶端 A: 執行 fg"
echo "5. 觀察：A 收到所有消息"
read -p "按 Enter 繼續..."
```

---

## 🎯 評分時的用途

### Ctrl+D - 必考項目

**題目要求：**
> "try to send partial commands"

**測試方法：**
```bash
PRI[Ctrl+D]VMG[Ctrl+D] #test :msg[Enter]
```

**評分點：**
- ✅ 伺服器正確累積部分命令
- ✅ 最終解析完整命令

### Ctrl+Z - 必考項目

**題目要求：**
> "Stop a client (^-Z) connected on a channel. Then flood the channel using another client."

**測試方法：**
```bash
# A 暫停，B 發大量消息，A 恢復
```

**評分點：**
- ✅ 伺服器不會 hang
- ✅ 消息正確累積
- ✅ 恢復後正常處理
- ✅ 無記憶體洩漏

### Ctrl+C - 可能測試

**題目要求：**
> "Unexpectedly kill a client"

**測試方法：**
```bash
[Ctrl+C] 或 kill -9 <pid>
```

**評分點：**
- ✅ 伺服器正確清理
- ✅ 其他客戶端不受影響

---

## 📋 記憶技巧

### 簡單記憶

```
Ctrl+D = Door（門）
  → 優雅地關門離開
  → 可能還會說「再見」

Ctrl+C = Cancel（取消）
  → 直接取消，走人
  → 門都不關

Ctrl+Z = Zzz（睡覺）
  → 睡著了，但還在房間
  → 可以叫醒
```

### 按鍵效果記憶

```
D = Done（完成）→ 輸入結束
C = Cancel（取消）→ 取消程序
Z = Zzz（打瞌睡）→ 暫停
```

---

## ⚠️ 常見誤解

### 誤解 1："Ctrl+D 會殺死程序"

**錯誤！**
- Ctrl+D 只是 EOF 字符
- 程序可能繼續運行（尤其是行中 Ctrl+D）
- Ctrl+**C** 才會殺死程序

### 誤解 2："Ctrl+Z 會關閉連接"

**錯誤！**
- Ctrl+Z 只是暫停程序
- 連接仍然存在
- fg 可以恢復

### 誤解 3："Ctrl+C 和 Ctrl+D 效果一樣"

**完全不同！**
- Ctrl+D = 文件結束（可能優雅關閉）
- Ctrl+C = 殺死程序（強制終止）

---

## 📖 快速參考卡

### 控制鍵速查表

```
┌────────────────────────────────────────────────┐
│ Ctrl+D - EOF 文件結束                           │
├────────────────────────────────────────────────┤
│ 作用：結束輸入                                  │
│ 行中：發送內容，不關閉                          │
│ 空行：發送 EOF，優雅關閉                        │
│ 用途：測試部分命令、EOF 處理                    │
└────────────────────────────────────────────────┘

┌────────────────────────────────────────────────┐
│ Ctrl+C - SIGINT 中斷信號                        │
├────────────────────────────────────────────────┤
│ 作用：立即終止程序                              │
│ 效果：強制關閉，立即退出                        │
│ 用途：強制退出、測試異常斷開                    │
└────────────────────────────────────────────────┘

┌────────────────────────────────────────────────┐
│ Ctrl+Z - SIGTSTP 暫停信號                       │
├────────────────────────────────────────────────┤
│ 作用：暫停程序                                  │
│ 效果：連接保持，可用 fg 恢復                    │
│ 用途：測試非阻塞 I/O、緩衝區管理                │
└────────────────────────────────────────────────┘
```

---

## ✅ 總結

| | Ctrl+D | Ctrl+C | Ctrl+Z |
|---|--------|--------|--------|
| **是什麼** | EOF 字符 | 中斷信號 | 暫停信號 |
| **程序** | 可能繼續 | 終止 | 暫停 |
| **連接** | 關閉寫端 | 強制關閉 | 保持 |
| **可恢復** | ❌ 寫端不能 | ❌ | ✅ fg |
| **評分用途** | 部分命令 + EOF | 異常斷開 | 非阻塞測試 |

**關鍵：三個完全不同的東西，測試不同的伺服器特性！** 🎯
