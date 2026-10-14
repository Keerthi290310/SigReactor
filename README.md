# SigReactor — A Signal-Driven Event Framework

SigReactor is a Linux-based event-driven framework written in C11.
It combines I/O readiness, timers, signals, and process management
through a single epoll-based event loop.

## Project Objective

The objective of SigReactor is to provide a simple event framework
that can efficiently handle multiple asynchronous events without
using separate blocking loops for each event source.

## Core Features

- Linux epoll-based event loop
- TCP server and client communication
- Multiple client handling
- Level-triggered event monitoring
- Edge-triggered reactor API support
- signalfd-based signal handling
- SIGINT and SIGTERM graceful shutdown
- SIGCHLD child-process monitoring
- Child-process creation and reaping
- Periodic timer support
- One-shot timer support
- EINTR-safe system calls
- Clean resource shutdown
- Stress-testing support

## Architecture

```text
                    +----------------------+
                    |      SigReactor      |
                    |     Event Loop       |
                    +----------+-----------+
                               |
              +----------------+----------------+
              |                |                |
              v                v                v
        +-----------+    +-----------+    +-----------+
        | TCP I/O   |    |  Timers   |    | Signals   |
        | epoll     |    | timerfd   |    | signalfd  |
        +-----------+    +-----------+    +-----------+
              |                |                |
              +----------------+----------------+
                               |
                               v
                    +----------------------+
                    | Process Management   |
                    | fork / exec / wait   |
                    +----------------------+
