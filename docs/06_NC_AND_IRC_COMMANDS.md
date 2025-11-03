# nc 參數與 IRC 命令詳解

## 🔧 netcat (nc) 參數說明

### `-C` 參數：CRLF 行結束

**作用：** 將行結束符轉換為 CRLF（`\r\n`）

```bash
# 不使用 -C
nc localhost 6667
NICK alice[Enter]
# 發送: "NICK alice\n" (只有 LF)

# 使用 -C
nc -C localhost 6667
NICK alice[Enter]
# 發送: "NICK alice\r\n" (CRLF)
```

#### 為什麼重要？

**IRC 協議標準：**
- RFC 1459 規定：IRC 命令必須以 `\r\n` (CRLF) 結尾
- `\r` = Carriage Return（回車，ASCII 13）
- `\n` = Line Feed（換行，ASCII 10）

**系統差異：**
```
Unix/Linux/macOS:  行結束符是 \n (LF)
Windows:           行結束符是 \r\n (CRLF)
IRC 協議:          要求 \r\n (CRLF)
```

**我們的伺服器處理：**
```cpp
// Client::popLine() - 容錯處理
std::string::size_type pos = _inbuf.find("\r\n");  // 優先尋找 CRLF
if (pos != std::string::npos) {
    // 找到 CRLF，標準 IRC
}
// 寬鬆：也接受只有 '\n'
std::string::size_type lf = _inbuf.find('\n');
if (lf != std::string::npos) {
    // 也可以工作，為了相容測試工具
}
```

#### 何時使用 -C

| 場景 | 是否使用 -C | 原因 |
|------|-----------|------|
| **測試自己的伺服器** | 建議使用 | 符合 IRC 標準 |
| **連接真實 IRC 伺服器** | **必須使用** | 否則可能無法識別命令 |
| **腳本自動化** | 建議使用 | 確保相容性 |
| **快速測試** | 可選 | 我們的伺服器容錯處理 |

---

### `-q` 參數：EOF 後延遲時間

**作用：** 控制 stdin EOF 後的等待時間

```bash
# nc -q 0
nc -q 0 localhost 6667
# EOF 後等待 0 秒，立即關閉連接

# nc -q 5
nc -q 5 localhost 6667
# EOF 後等待 5 秒，才關閉連接

# nc（無 -q）
nc localhost 6667
# 預設行為（各版本不同）
```

#### 詳細行為

```
場景：使用管道輸入命令
─────────────────────────────────────────────────────

不使用 -q（或預設值）：
echo "NICK alice" | nc localhost 6667
     ↓
nc 發送 "NICK alice\n"
     ↓
stdin EOF（管道結束）
     ↓
[等待...] （預設可能等 10 秒或永遠）
     ↓
（可能收不到伺服器回應就關閉了）

使用 -q 0：
echo "NICK alice" | nc -q 0 localhost 6667
     ↓
nc 發送 "NICK alice\n"
     ↓
stdin EOF
     ↓
等待 0 秒 ← 立即關閉
     ↓
連接關閉

使用 -q 3：
echo "NICK alice" | nc -q 3 localhost 6667
     ↓
nc 發送 "NICK alice\n"
     ↓
stdin EOF
     ↓
等待 3 秒 ← 有時間接收伺服器回應
     ↓
收到: ":server 001 alice :Welcome..."
     ↓
3 秒後關閉
```

#### 何時使用 -q

| 場景 | 建議參數 | 原因 |
|------|---------|------|
| **互動式測試** | 不需要 -q | 手動輸入，不會立即 EOF |
| **管道輸入** | `-q 2` 或 `-q 3` | 給時間接收回應 |
| **測試 EOF** | `-q 0` | 立即觸發 EOF 行為 |
| **自動化腳本** | `-q 1` | 確保收到回應 |

---

### `-C` 和 `-q` 結合使用

```bash
# 最佳實踐：互動式測試
nc -C localhost 6667
# -C: 使用 CRLF（符合 IRC 標準）
# 不用 -q: 手動操作不需要

# 自動化腳本
echo -e "PASS password123\nNICK alice\nUSER alice 0 * :Alice" | nc -C -q 2 localhost 6667
# -C: CRLF 行結束
# -q 2: EOF 後等 2 秒接收回應
```

---

## 📝 IRC USER 命令詳解

### 命令格式

```
USER <username> <mode> <unused> :<realname>
```

### 參數說明

#### 完整範例
```bash
USER alice 0 * :Alice Smith
     │     │ │  │
     │     │ │  └─ realname（真實姓名）
     │     │ └──── unused（未使用，通常是 *）
     │     └────── mode（用戶模式，通常是 0）
     └──────────── username（用戶名）
```

### 各參數詳解

#### 1. `<username>` - 用戶名

```
作用：識別用戶的帳號名稱
格式：字母、數字、某些符號
範例：alice, bob123, test_user

注意：
- 與 NICK（暱稱）不同
- username 通常顯示在完整前綴中
```

**範例：**
```
NICK: alice
USER: alice_user

完整前綴：alice!alice_user@hostname
         ↑     ↑
         暱稱   用戶名
```

#### 2. `<mode>` - 用戶模式（通常設為 0）

```
作用：設定用戶的初始模式標誌
格式：數字（位元遮罩）

常見值：
- 0  : 無特殊模式（最常見）
- 8  : invisible（隱身）
- 4  : wallops
```

**實際使用：**
```bash
USER alice 0 * :Alice    # ← 最常見，無特殊模式
USER bob 8 * :Bob        # ← 設為隱身模式
```

**我們的實現：**
```cpp
// cmd_user.cpp
const std::string username = m.params[0];
const std::string realname = m.params[3];
// mode 參數（m.params[1]）通常被忽略
// 因為現代 IRC 客戶端不太使用這個
```

#### 3. `<unused>` - 未使用參數（通常是 `*`）

```
作用：歷史遺留參數，現在不使用
格式：通常填 * 或主機名

歷史：
- 早期 IRC 用來傳遞主機名
- 現代 IRC 伺服器自動檢測主機名
- 所以這個參數變成「未使用」
```

**範例：**
```bash
USER alice 0 * :Alice        # ← 標準用法
USER bob 0 localhost :Bob    # ← 也可以，但會被忽略
USER charlie 0 unused :C     # ← 任何值都可以
```

**我們的處理：**
```cpp
// cmd_user.cpp
const std::string hostname = m.params[2];  
if (hostname != "*")  // 只在不是 * 時使用
    cl.setHost(hostname);
// 否則使用伺服器自動檢測的 IP
```

#### 4. `:<realname>` - 真實姓名

```
作用：用戶的完整名稱或描述
格式：冒號開頭，可以包含空格
範例：:Alice Smith, :Bob from Paris, :測試用戶

注意：
- 必須以 : 開頭（表示這是最後一個參數，可以有空格）
- 可以是任何文字
- 顯示在 WHOIS 等命令的回應中
```

**範例：**
```bash
USER alice 0 * :Alice Smith          # 完整姓名
USER bob 0 * :IRC Test User          # 描述
USER charlie 0 * :Charlie from NYC   # 帶地點
USER dave 0 * :测试用户              # 中文也可以
```

### USER 命令完整範例

#### 最常見用法

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice Smith
```

**解析：**
```
USER alice 0 * :Alice Smith
     ↑     ↑ ↑  ↑
     │     │ │  └─ realname = "Alice Smith"
     │     │ └──── unused = "*" (標準做法)
     │     └────── mode = 0 (無特殊模式)
     └──────────── username = "alice"

結果：
nick     = alice
username = alice
realname = Alice Smith
host     = 127.0.0.1 (伺服器自動檢測)

完整前綴：alice!alice@127.0.0.1
```

#### 變化形式

```bash
# 範例 1: 不同的 username 和 nick
NICK AliceWonderland
USER alice_w 0 * :Alice from Wonderland
# 前綴：AliceWonderland!alice_w@127.0.0.1

# 範例 2: 使用模式標誌
USER bob 8 * :Bob the Hidden
# mode=8 表示隱身（但我們的實現可能不處理）

# 範例 3: 提供主機名（通常被忽略）
USER charlie 0 localhost :Charlie
# 第三個參數是 localhost，但伺服器會用實際 IP
```

---

## 🎯 參數組合建議

### 最佳實踐

#### 互動式測試（推薦）
```bash
nc -C localhost 6667
# 只用 -C 就夠了
# 手動輸入不需要 -q
```

#### 自動化腳本
```bash
(
  echo "PASS password123"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  sleep 2
  echo "JOIN #test"
  echo "PRIVMSG #test :Hello"
  sleep 1
  echo "QUIT :Bye"
) | nc -C -q 3 localhost 6667
# -C: CRLF 行結束
# -q 3: EOF 後等 3 秒接收回應
```

#### 測試 EOF 行為
```bash
nc -C -q 0 localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
[Ctrl+D]
# -q 0: 立即關閉，用於測試伺服器的 EOF 處理
```

---

## 📊 完整註冊流程範例

### 標準註冊

```bash
nc -C localhost 6667

# 步驟 1: 密碼驗證
PASS password123
# 伺服器內部：cl.setPassOk()

# 步驟 2: 設定暱稱
NICK alice
# 伺服器內部：cl.setNick("alice")

# 步驟 3: 設定用戶資訊
USER alice 0 * :Alice Smith
# 伺服器內部：
# cl.setUser("alice", "Alice Smith")
# cl.tryFinishRegister() → 檢查 hasPass && hasNick && hasUser
# 發送歡迎消息！

# 收到回應
:ft_irc 001 alice :Welcome to the ft_irc network, alice
```

### USER 命令的內部處理

```cpp
// cmd_user.cpp
void CmdUser::execute(Server& srv, Client& cl, const IrcMessage& m) {
    // 檢查參數數量
    if (m.params.size() < 4) {
        cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "USER"));
        return;
    }
    
    // 解析參數
    const std::string username = m.params[0];  // "alice"
    // m.params[1] 是 mode (0)
    // m.params[2] 是 unused (*)
    const std::string realname = m.params[3];  // "Alice Smith"
    
    // 設定用戶資訊
    cl.setUser(username, realname);
    
    // 檢查是否完成註冊
    if (cl.tryFinishRegister()) {
        srv.sendWelcome(cl);  // 發送 001 歡迎消息
    }
}
```

---

## 🔍 nc 參數對比表

| 參數 | 作用 | 何時使用 | 影響 |
|------|------|---------|------|
| **-C** | 發送 CRLF (`\r\n`) | 連接 IRC 伺服器時 | 確保命令被正確識別 |
| **-q 0** | EOF 後等待 0 秒 | 測試 EOF 處理 | 立即關閉 |
| **-q 2** | EOF 後等待 2 秒 | 管道輸入 | 有時間接收回應 |
| **-v** | 詳細模式 | 調試連接 | 顯示連接資訊 |
| **-N** | shutdown write after EOF | 測試半關閉 | 只關閉寫，讀仍開啟 |

---

## 📋 USER 命令變化範例

### 範例 1：最簡單形式

```bash
USER test 0 * :Test User
```

**解析：**
- username: `test`
- mode: `0`
- unused: `*`
- realname: `Test User`

### 範例 2：長 realname

```bash
USER alice 0 * :Alice Smith from New York, USA
```

**解析：**
- username: `alice`
- realname: `Alice Smith from New York, USA`（可以很長）

### 範例 3：中文 realname

```bash
USER phoenix 0 * :鳳凰使用者
```

**解析：**
- username: `phoenix`
- realname: `鳳凰使用者`

### 範例 4：錯誤示範

```bash
# ❌ 錯誤 1：缺少冒號
USER alice 0 * Alice Smith
# 伺服器會把 "Alice" 當作 realname
# "Smith" 會被當作額外參數

# ❌ 錯誤 2：參數不足
USER alice
# 缺少必要參數，伺服器回應：
# :server 461 USER :Not enough parameters

# ❌ 錯誤 3：順序錯誤
USER alice 0 * :Alice    # 先 USER
PASS password123         # 後 PASS
# 伺服器拒絕（必須先 PASS）
# :server 464 :Password incorrect
```

---

## 🎯 完整測試腳本

### 腳本 1：互動式測試（推薦）

```bash
#!/bin/bash
# test_interactive.sh

echo "=== 互動式 IRC 測試 ==="
echo "使用 -C 確保 CRLF"
echo ""
echo "連接後依序輸入："
echo "  PASS password123"
echo "  NICK yourname"
echo "  USER yourname 0 * :Your Real Name"
echo ""

nc -C localhost 6667
```

### 腳本 2：自動化測試

```bash
#!/bin/bash
# test_auto.sh

echo "=== 自動化測試（使用 -q 3）==="

(
  echo "PASS password123"
  sleep 0.5
  echo "NICK testbot"
  sleep 0.5
  echo "USER testbot 0 * :Test Bot User"
  sleep 1
  echo "JOIN #test"
  sleep 0.5
  echo "PRIVMSG #test :Hello from script!"
  sleep 0.5
  echo "QUIT :Goodbye"
) | nc -C -q 3 localhost 6667

echo ""
echo "測試完成"
```

### 腳本 3：測試不同 USER 格式

```bash
#!/bin/bash
# test_user_formats.sh

echo "=== 測試 USER 命令各種格式 ==="

# 測試 1: 標準格式
echo "測試 1: 標準格式"
(
  echo "PASS password123"
  echo "NICK test1"
  echo "USER test1 0 * :Test User One"
  sleep 2
) | nc -C -q 1 localhost 6667
echo ""

# 測試 2: 長 realname
echo "測試 2: 長 realname"
(
  echo "PASS password123"
  echo "NICK test2"
  echo "USER test2 0 * :This is a very long real name with many words"
  sleep 2
) | nc -C -q 1 localhost 6667
echo ""

# 測試 3: 中文 realname
echo "測試 3: 中文 realname"
(
  echo "PASS password123"
  echo "NICK test3"
  echo "USER test3 0 * :測試用戶三號"
  sleep 2
) | nc -C -q 1 localhost 6667
echo ""
```

---

## 🔑 關鍵區別總結

### nc -C vs 無 -C

```
情境：發送 NICK alice

無 -C:
nc localhost 6667
NICK alice[Enter]
→ 發送: "NICK alice\n"
→ 某些嚴格的 IRC 伺服器可能拒絕

有 -C:
nc -C localhost 6667
NICK alice[Enter]
→ 發送: "NICK alice\r\n"
→ 符合 RFC 標準 ✅
```

### nc -q 0 vs -q 3 vs 無 -q

```
使用管道：echo "NICK alice" | nc ...

-q 0:
→ 發送命令
→ stdin EOF
→ 立即關閉（0 秒）
→ 可能收不到回應 ❌

-q 3:
→ 發送命令
→ stdin EOF
→ 等待 3 秒
→ 收到伺服器回應 ✅
→ 3 秒後關閉

無 -q:
→ 發送命令
→ stdin EOF
→ 等待（預設時間，因版本而異）
→ 行為不確定
```

---

## 💡 實用建議

### 給評分時使用

**展示基本功能：**
```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice Smith
JOIN #test
PRIVMSG #test :Hello everyone!
QUIT :Goodbye
```

**測試部分命令（Ctrl+D）：**
```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
# 測試分段
PRI[Ctrl+D]VMG[Ctrl+D] #test :msg[Enter]
```

**測試 EOF 處理：**
```bash
nc -C -q 0 localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
[Ctrl+D]  # 立即觸發 EOF
```

### 給自動化測試

```bash
# 完整註冊流程測試
cat > commands.txt << 'EOF'
PASS password123
NICK alice
USER alice 0 * :Alice Smith
JOIN #test
PRIVMSG #test :Hello
QUIT :Bye
EOF

cat commands.txt | nc -C -q 2 localhost 6667
```

---

## 📖 USER 命令的完整前綴

### 前綴格式

```
:nick!username@hostname

範例：
:alice!alice_user@127.0.0.1
:bob!bob123@192.168.1.100
:charlie!char@example.com
```

### 如何顯示

```bash
# 當 alice 發送消息時
PRIVMSG #test :Hello

# 其他頻道成員看到：
:alice!alice@127.0.0.1 PRIVMSG #test :Hello
 ↑     ↑     ↑
 NICK  USER  HOST
```

### 在 WHOIS 中

```bash
WHOIS alice

# 回應包含：
311 alice alice 127.0.0.1 * :Alice Smith
    ↑     ↑     ↑            ↑
    nick  user  host         realname
```

---

## ✅ 快速檢查表

**測試時應該使用：**
```
□ nc -C localhost 6667           (互動式測試)
□ nc -C -q 2 ... (管道輸入測試)
□ nc -C -q 0 ... (EOF 測試)
```

**USER 命令格式：**
```
□ USER <username> 0 * :<realname>
□ realname 前面要有冒號
□ 參數要有 4 個
□ 在 PASS 和 NICK 之後
```

---

**現在您應該完全理解 nc 參數和 USER 命令了！** 🎉

