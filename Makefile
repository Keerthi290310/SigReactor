CC = gcc
CFLAGS = -D_DEFAULT_SOURCE -Wall -Wextra -std=c11 -Iinclude

all: sigreactor_server sigreactor_client test_timer tests/test_handler tests/test_process demo/process_demo

sigreactor_server: src/main.c src/reactor.c src/timer.c src/signal.c src/process.c src/handler.c
	$(CC) $(CFLAGS) src/main.c src/reactor.c src/timer.c src/signal.c src/process.c src/handler.c -o sigreactor_server

sigreactor_client: demo/client.c
	$(CC) $(CFLAGS) demo/client.c -o sigreactor_client

test_timer: tests/test_timer.c src/timer.c
	$(CC) $(CFLAGS) tests/test_timer.c src/timer.c -o test_timer

tests/test_handler: tests/test_handler.c src/handler.c
	$(CC) $(CFLAGS) tests/test_handler.c src/process.c src/handler.c -o tests/test_handler

tests/test_process: tests/test_process.c src/process.c src/handler.c
	$(CC) $(CFLAGS) tests/test_process.c src/process.c src/handler.c -o tests/test_process

demo/process_demo: demo/process_demo.c src/process.c src/handler.c
	$(CC) $(CFLAGS) demo/process_demo.c src/process.c src/handler.c -o demo/process_demo

clean:
	rm -f sigreactor_server sigreactor_client test_timer tests/test_handler tests/test_process demo/process_demo
