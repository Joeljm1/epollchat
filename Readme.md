# Epoll Chat

A TCP chat server written in C++ using Linux `epoll` for an asynchronous event loop.

## Overview

The server uses non-blocking I/O and an `epoll` event loop to handle multiple client connections within a single thread. Clients connect via TCP, send text messages, and have their messages broadcasted to others in the same room.

Key points:
- Non-blocking sockets with edge/level monitoring via `epoll`.
- Single-threaded event loop handling connection accept, read, and write events.
- Per-client output queues to handle partial writes and socket backpressure without blocking the loop.
- Line-buffered message parsing (messages are broadcast when a newline is received).

## Requirements

- Linux (requires `epoll` and `accept4`)
- C++23 capable compiler (e.g. GCC 13+ or Clang 16+)
- CMake 3.22 or newer

## Building

```bash
cmake -B build
cmake --build build
```

## Running

Start the server:

```bash
./build/src/epoll_chat
```

By default, the server listens on `127.0.0.1:12345`.

You can connect using any standard TCP client like netcat or telnet:

```bash
nc 127.0.0.1 12345
```

Open multiple terminal windows with `nc` to chat between clients.

## TODO

- [ ] User commands (e.g. `/nick`, `/join`, `/leave`, `/list`, `/msg`)
- [ ] User authentication and registration (usernames, passwords, persistent sessions)
- [ ] Multiple chat rooms and room passwords
- [ ] Proper error handling, buffer size limits, and rate limiting to avoid memory exhaustion
- [ ] Dedicated client application (CLI or TUI)
- [ ] Clean server shutdown handling on signals like SIGINT/SIGTERM
