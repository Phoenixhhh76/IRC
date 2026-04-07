# IRC — IRC Server in C++ (poll-based)

Educational **IRC-style server** implemented in **C++** to practice low-level backend fundamentals: **TCP sockets**, **IRC-like protocol parsing**, **non-blocking multi-client I/O with `poll()`**, and **stateful server design**.

## What I learned / what this project demonstrates 

### Networking (TCP sockets)
- Built a TCP server that accepts and manages multiple concurrent client connections.
- Implemented stream-safe communication: **buffering**, **partial reads/writes**, and message boundary handling.
- Handled real-world connection edge cases (disconnects, invalid input, broken pipes/timeouts depending on config).

### Event-driven concurrency with `poll()`
- Used **`poll()`** to multiplex I/O across many sockets in a single event loop.
- Avoided blocking behavior by tracking per-client read/write buffers and socket readiness.
- Designed logic to scale beyond a single client without spawning one thread per connection.

### Protocol design & parsing
- Implemented an IRC-like command pipeline: **parse → validate → execute → respond**.
- Built a robust parser for command + parameters and returned clear server error replies when input is invalid.
- Ensured consistency in server responses so clients can reliably interpret results.

### State management (server-side models)
- Modeled server entities such as:
  - clients/users (nickname, registration state, connection info)
  - channels (members, topic/modes if implemented)
  - message routing (private vs channel broadcast)
- Ensured correct state transitions on JOIN/PART/QUIT and on unexpected disconnects.

### Engineering practices
- Modularized the codebase (network loop, parsing, command handlers, models/data structures).
- Used Git for incremental delivery and debugging.
- Added logging/debug output to trace protocol flow and connection lifecycle.

## Key features (update to match your implementation)
- Registration/auth: `PASS`, `NICK`, `USER`
- Channels: `JOIN`, `PART` (optional: `TOPIC`, `INVITE`, `KICK`, `MODE`)
- Messaging: private messages and channel broadcast
- Validation + error handling for malformed commands and invalid states

## Tech stack
- **Language:** C++
- **I/O model:** event-driven server using **`poll()`**
- **Networking:** POSIX sockets (TCP)
- **Build:** [Make / CMake]
- **Platform:** [Linux/macOS]

## How to build and run (fill in with your real commands)

### Build
```bash
# example
make
```

### Run server
```bash
# example — replace with your actual binary name/args
./[binary_name] <port> <password>
```

### Connect (example)
```bash
# netcat example
nc 127.0.0.1 <port>
```

## Resume bullets (copy/paste)
- Implemented an IRC-style chat server in **C++** using **TCP sockets** and **`poll()`** for non-blocking, multi-client I/O.
- Built a command-processing pipeline (parse/validate/execute/respond) with defensive parsing and clear error handling.
- Designed server-side state management for clients and channels, supporting message routing and reliable handling of disconnects.

## Author
- Phoenixhhh76
# irc_test cmd
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --num-callers=20 --track-fds=yes \
  --log-file=valgrind.log \
  ./ircserv 6667 abc

