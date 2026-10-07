# SigReactor — A Signal-Driven Event Framework

## Overview

SigReactor is a Linux-based event-driven framework written in C11.

It provides a unified epoll-based event loop for I/O readiness, timers, signals, and child processes.

## Key Features

- epoll-based event loop
- TCP client/server communication
- Periodic timers using timerfd
- Safe signal handling using signalfd
- SIGINT and SIGTERM clean shutdown
- SIGCHLD child-process notification
- Child-process creation and reaping
- EINTR handling
- Clean resource cleanup

## Build

    make clean
    make

## Run Server

    ./sigreactor_server

## Run Client

    ./sigreactor_client

## Tests

    ./test_timer
    ./tests/test_handler
    ./tests/test_process
    ./demo/process_demo

## Technologies

- C11
- Linux
- epoll
- timerfd
- signalfd
- TCP sockets
- Processes
- IPC
- Concurrency

## Team

- G. Keerthi — 2500032083
- T. Sahasra — 2500032211
- K. Nansy — 2500032155

## Project

**SigReactor — A Signal-Driven Event Framework**

OS & System Programming PBL
Department of CSE
