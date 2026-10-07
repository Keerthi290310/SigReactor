#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "reactor.h"
#include "timer.h"
#include "process.h"

#define PORT 8080
#define MAX_EVENTS 10
#define TIMER_INTERVAL_MS 5000

int main(void)
{
    int server_fd;
    int epoll_fd;
    int timer_fd;
    int signal_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int reuse = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    epoll_fd = reactor_create();

    if (epoll_fd == -1)
    {
        perror("reactor_create");
        close(server_fd);
        return 1;
    }

    if (reactor_add(epoll_fd, server_fd) == -1)
    {
        perror("reactor_add server");
        reactor_destroy(epoll_fd);
        close(server_fd);
        return 1;
    }

    timer_fd = reactor_timer_create(TIMER_INTERVAL_MS, 1);

    if (timer_fd == -1)
    {
        perror("reactor_timer_create");
        reactor_destroy(epoll_fd);
        close(server_fd);
        return 1;
    }

    if (reactor_add(epoll_fd, timer_fd) == -1)
    {
        perror("reactor_add timer");
        reactor_timer_destroy(timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);
        return 1;
    }

    signal_fd = reactor_add_signal(epoll_fd);

    if (signal_fd == -1)
    {
        perror("reactor_add_signal");
        reactor_timer_destroy(timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);
        return 1;
    }

    printf("====================================\n");
    printf("       SigReactor Server\n");
    printf("====================================\n");
    printf("Port       : %d\n", PORT);
    printf("Timer      : %d ms periodic\n", TIMER_INTERVAL_MS);
    printf("Signals    : SIGINT / SIGTERM / SIGCHLD\n");
    printf("Event loop : epoll\n");
    printf("Waiting for events...\n\n");

    char *child_args[] = {
        "sh",
        "-c",
        "exit 0",
        NULL
    };

    pid_t child_pid = reactor_spawn("sh", child_args);

    if (child_pid == -1)
    {
        fprintf(stderr,
                "[Process] Failed to spawn child process\n");
    }
    else
    {
        printf("[Process] Spawned child PID=%ld\n",
               (long)child_pid);
    }

    while (1)
    {
        struct epoll_event events[MAX_EVENTS];

        int count = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < count; i++)
        {
            int fd = events[i].data.fd;

            if (fd == signal_fd)
            {
                int signal_number;

                if (reactor_read_signal(&signal_number) == -1)
                {
                    perror("reactor_read_signal");
                    continue;
                }

                if (signal_number == SIGCHLD)
                {
                    int reaped = reactor_reap_children();

                    printf("[SIGCHLD] Reaped %d child process(es)\n",
                           reaped);

                    continue;
                }

                if (signal_number == SIGINT ||
                    signal_number == SIGTERM)
                {
                    printf("\nShutdown signal received.\n");
                    goto shutdown;
                }

                continue;
            }

            if (fd == timer_fd)
            {
                uint64_t expirations = 0;

                if (reactor_timer_read(
                        timer_fd,
                        &expirations) == 0)
                {
                    printf("[Timer] %llu expiration(s)\n",
                           (unsigned long long)expirations);
                }

                continue;
            }

            if (fd == server_fd)
            {
                int client_fd = accept(
                    server_fd,
                    NULL,
                    NULL
                );

                if (client_fd == -1)
                {
                    if (errno == EINTR)
                        continue;

                    perror("accept");
                    continue;
                }

                printf("Client connected: fd=%d\n",
                       client_fd);

                if (reactor_add(
                        epoll_fd,
                        client_fd) == -1)
                {
                    perror("reactor_add client");
                    close(client_fd);
                }

                continue;
            }

            char buffer[1024];

            ssize_t bytes_read = read(
                fd,
                buffer,
                sizeof(buffer) - 1
            );

            if (bytes_read > 0)
            {
                buffer[bytes_read] = '\0';

                printf("Received: %s\n", buffer);

                ssize_t bytes_sent = write(
                    fd,
                    buffer,
                    (size_t)bytes_read
                );

                if (bytes_sent == -1)
                {
                    perror("write");
                    close(fd);
                }
                else
                {
                    printf("Response sent\n");
                }
            }
            else if (bytes_read == 0)
            {
                printf("Client disconnected: fd=%d\n",
                       fd);

                close(fd);
            }
            else
            {
                if (errno == EINTR)
                    continue;

                perror("read");
                close(fd);
            }
        }
    }

shutdown:

    printf("Cleaning up...\n");

    reactor_timer_destroy(timer_fd);
    reactor_destroy(epoll_fd);
    close(server_fd);

    printf("SigReactor stopped.\n");

    return 0;
}
