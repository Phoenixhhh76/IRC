# Bonus 功能說明（Bot 與 DCC）

## 目錄
- Bot 功能
- DCC 檔案傳輸
- 舊版 DCC 問題總結
- 新版 DCC 改善總結
- 編譯與目錄結構

---

## Bot 功能
- 內建 Bot 暱稱：`ft_irc_Bot`
- 觸發方式：
  - 私訊：`PRIVMSG ft_irc_Bot :!help`
  - 頻道：在任何 `#channel` 送出 `!hello`、`!help`、`!time`、`!stats`
- 回覆行為：
  - `!hello`：問候
  - `!help`：顯示可用指令
  - `!time`：顯示伺服器時間
  - `!stats`：目前使用者與頻道統計

實作位置：
- `src/bonus/Bot.cpp`
- 介面定義：`Server` 類別中的 `initBot`、`handleBotMessage`、`botSendMessage`

---

## DCC 檔案傳輸（接收端）
- 指令格式：`DCC SEND <filename> <IP> <port>`
- 行為：
  - 伺服器以非阻塞 TCP 連線至 `<IP>:<port>` 接收資料
  - 儲存位置：`/tmp/ftirc_<timestamp>_<filename>`
  - 完成或中止時，以 `NOTICE` 通知發起者
- 非阻塞與單一 `poll()`：
  - 連線使用 `EINPROGRESS` 模式，以 `POLLOUT` 確認完成
  - 收資料在 `POLLIN` 事件中持續 `recv`，寫入檔案

實作位置：
- `src/bonus/Dcc.cpp`
- 介面定義：`Server` 類別中的 `handleDccSend`、`processDccPollEvent`
- 會話結構：`include/bonus/Dcc.hpp` 中的 `DccSession`

---

## 舊版 DCC 問題總結
- 將 DCC socket 當作一般 IRC 用戶端 FD 處理：
  - 在 `POLLIN` 走 `readFromSocket()` 嘗試解析 IRC 文本，DCC 原始資料導致解析失敗 → 被判定連線關閉。
  - 在 `POLLOUT` / `POLLERR` / `POLLHUP` 未分流，DCC 結束被誤當主連線斷線。
- 關閉流程未區分 DCC 與主連線：
  - `closeClient()` 對任何 FD 都做用戶端清理，可能錯誤移除/關閉發起者主連線，或在 `swap/pop` 後影響其他 `poll` 項目。
- 錯誤與阻塞處理不當：
  - DCC `connect/recv` 錯誤或 EOF 被當成 IRC 會話錯誤處理，連帶關閉主連線。

---

## 新版 DCC 改善總結
- DCC 與主連線明確分流：
  - 以 `_dccByFd` 辨識 DCC FD；`poll` 事件委派至 `processDccPollEvent`，不走 IRC 文本管線。
- 安全的關閉與回報：
  - `closeClient()` 先檢查 DCC：只關 DCC 檔案與 socket，`NOTICE` 回報完成/中止，直接返回，不觸碰發起者主 FD。
- 全程非阻塞與單一 `poll`：
  - `connect` 非阻塞（`EINPROGRESS` 後以 `POLLOUT` 確認），資料 `recv` 後寫檔；錯誤/EOF 僅影響該 DCC 會話。
- 行為隔離：
  - 未使用 DCC 指令時，不涉入任何新路徑；一般註冊、頻道、私訊完全不受影響。

---

## 編譯與目錄結構
- 主要變更：
  - 新增 `include/bonus/Dcc.hpp`、`src/bonus/Bot.cpp`、`src/bonus/Dcc.cpp`
  - `Makefile` 已加入上述檔案至建置流程
- 相關路徑：
  - Bot：`src/bonus/Bot.cpp`
  - DCC：`include/bonus/Dcc.hpp`、`src/bonus/Dcc.cpp`
  - Server 介面：`include/Server.hpp`（內含 `_dccByFd` 與介面宣告）

如需更保守地控制 Bonus，可加上條件編譯旗標（例如 `-DENABLE_BONUS`）再包起來，以便完全關閉 Bonus 程式碼以對齊 Mandatory 測試行為。
