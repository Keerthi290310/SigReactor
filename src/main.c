#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <signal.h>

#include "reactor.h"
#include "timer.h"
#include "process.h"

#define PORT 8080
#define MAX_EVENTS 32
#define BUFFER_SIZE 1024

#define PERIODIC_TIMER_MS 5000
#define ONE_SHOT_TIMER_MS 10000

static int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int send_response(int client_fd)
{
    const char *response = "Hello from server\n";
    size_t total_sent = 0;
    size_t response_length = strlen(response);

    while (total_sent < response_length)
    {
        ssize_t n = send(client_fd,
                         response + total_sent,
                         response_length - total_sent,
                         0);

        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (n == 0)
            return -1;

        total_sent += (size_t)n;
    }

    printf("Response sent: %s", response);
    return 0;
}

static void close_client(int epoll_fd, int client_fd)
{
    epoll_ctl(epoll_fd,
              EPOLL_CTL_DEL,
              client_fd,
              NULL);

    close(client_fd);
}

int main(void)
{
    int server_fd = -1;
    int epoll_fd = -1;
    int periodic_timer_fd = -1;
    int one_shot_timer_fd = -1;
    int signal_fd = -1;

    int running = 1;

    struct sockaddr_in server_addr;
    struct epoll_event events[MAX_EVENTS];

    printf("====================================\n");
    printf("       SigReactor Server\n");
    printf("====================================\n");
    printf("Port            : %d\n", PORT);
    printf("Periodic Timer  : %d ms\n", PERIODIC_TIMER_MS);
    printf("One-Shot Timer  : %d ms\n", ONE_SHOT_TIMER_MS);
    printf("Signals         : SIGINT / SIGTERM / SIGCHLD\n");
    printf("Event loop      : epoll\n");
    printf("Mode            : Signal Priority + Graceful Shutdown\n");
    printf("Waiting for events...\n\n");

    /* Create server socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    if (make_nonblocking(server_fd) == -1)
    {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

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

    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    /* Create reactor */
    epoll_fd = reactor_create();

    if (epoll_fd == -1)
    {
        close(server_fd);
        return 1;
    }

    if (reactor_add(epoll_fd, server_fd) == -1)
    {
        reactor_destroy(epoll_fd);
        close(server_fd);
        return 1;
    }

    /* Periodic timer */
    periodic_timer_fd =
        reactor_timer_create(PERIODIC_TIMER_MS, 1);

    if (periodic_timer_fd == -1)
    {
        perror("periodic timer");

        reactor_destroy(epoll_fd);
        close(server_fd);

        return 1;
    }

    if (reactor_add(epoll_fd,
                    periodic_timer_fd) == -1)
    {
        reactor_timer_destroy(periodic_timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);

        return 1;
    }

    /* One-shot timer */
    one_shot_timer_fd =
        reactor_timer_create(ONE_SHOT_TIMER_MS, 0);

    if (one_shot_timer_fd == -1)
    {
        perror("one-shot timer");

        reactor_timer_destroy(periodic_timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);

        return 1;
    }

    if (reactor_add(epoll_fd,
                    one_shot_timer_fd) == -1)
    {
        reactor_timer_destroy(one_shot_timer_fd);
        reactor_timer_destroy(periodic_timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);

        return 1;
    }

    /* Signal handling */
    signal_fd = reactor_add_signal(epoll_fd);

    if (signal_fd == -1)
    {
        fprintf(stderr,
                "Failed to initialize signal handling\n");

        reactor_timer_destroy(one_shot_timer_fd);
        reactor_timer_destroy(periodic_timer_fd);
        reactor_destroy(epoll_fd);
        close(server_fd);

        return 1;
    }

    /* Child process */
    char *child_args[] = {
        "sh",
        "-c",
        "sleep 1",
        NULL
    };

    pid_t child_pid =
        reactor_spawn("sh", child_args);

    if (child_pid > 0)
    {
        printf("[Process] Spawned child PID=%d\n",
               child_pid);
    }

    while (running)
    {
        int event_count;

        do
        {
            event_count =
                epoll_wait(epoll_fd,
                           events,
                           MAX_EVENTS,
                           -1);
        }
        while (event_count == -1 &&
               errno == EINTR);

        if (event_count == -1)
        {
            perror("epoll_wait");
            break;
        }

        /*
         * M12 SIGNAL PRIORITY
         *
         * Process signal events before normal
         * timer/client events.
         */
        for (int i = 0; i < event_count; i++)
        {
            if (events[i].data.fd == signal_fd)
            {
                int signal_number;

                if (reactor_read_signal(&signal_number) == 0)
                {
                    if (signal_number == SIGINT ||
                        signal_number == SIGTERM)
                    {
                        printf("\n");
                        printf("[Signal Priority] "
                               "Shutdown signal received\n");

                        running = 0;
                    }
                    else if (signal_number == SIGCHLD)
                    {
                        int reaped =
                            reactor_reap_children();

                        printf("[Signal Priority] "
                               "SIGCHLD: reaped %d child process(es)\n",
                               reaped);
                    }
                }
            }
        }

        /*
         * If shutdown was requested, skip normal
         * event processing and clean up.
         */
        if (!running)
            break;

        /* Process normal events */
        for (int i = 0; i < event_count; i++)
        {
            int fd = events[i].data.fd;

            if (fd == signal_fd)
                continue;

            /* Periodic timer */
            if (fd == periodic_timer_fd)
            {
                uint64_t expirations;

                if (reactor_timer_read(periodic_timer_fd,
                                       &expirations) == 0)
                {
                    printf("[Timer - Periodic] "
                           "%llu expiration(s)\n",
                           (unsigned long long)expirations);
                }

                continue;
            }

            /* One-shot timer */
            if (fd == one_shot_timer_fd)
            {
                uint64_t expirations;

                if (reactor_timer_read(one_shot_timer_fd,
                                       &expirations) == 0)
                {
                    printf("[Timer - One-Shot] "
                           "Triggered once\n");
                }

                epoll_ctl(epoll_fd,
                          EPOLL_CTL_DEL,
                          one_shot_timer_fd,
                          NULL);

                reactor_timer_destroy(one_shot_timer_fd);

                one_shot_timer_fd = -1;

                continue;
            }

            /* New clients */
            if (fd == server_fd)
            {
                while (1)
                {
                    int client_fd;

                    do
                    {
                        client_fd =
                            accept(server_fd,
                                   NULL,
                                   NULL);
                    }
                    while (client_fd == -1 &&
                           errno == EINTR);

                    if (client_fd == -1)
                    {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                            break;

                        perror("accept");
                        break;
                    }

                    if (make_nonblocking(client_fd) == -1)
                    {
                        close(client_fd);
                        continue;
                    }

                    printf("Client connected: fd=%d\n",
                           client_fd);

                    if (reactor_add(epoll_fd,
                                    client_fd) == -1)
                    {
                        close(client_fd);
                    }
                }

                continue;
            }

            /* Client socket */
            if (fd != periodic_timer_fd &&
                fd != one_shot_timer_fd)
            {
                char buffer[BUFFER_SIZE];

                ssize_t bytes_read;

                do
                {
                    bytes_read =
                        read(fd,
                             buffer,
                             sizeof(buffer) - 1);
                }
                while (bytes_read == -1 &&
                       errno == EINTR);

                if (bytes_read == -1)
                {
                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK)
                    {
                        perror("read");
                    }

                    close_client(epoll_fd, fd);
                    continue;
                }

                if (bytes_read == 0)
                {
                    printf("Client disconnected: fd=%d\n",
                           fd);

                    close_client(epoll_fd, fd);
                    continue;
                }

                buffer[bytes_read] = '\0';

                printf("Client fd=%d received: %s",
                       fd,
                       buffer);

                if (send_response(fd) == -1)
                {
                    close_client(epoll_fd, fd);
                    continue;
                }

                close_client(epoll_fd, fd);

                printf("Client completed: fd=%d\n",
                       fd);
            }
        }
    }

    /*
     * M12 GRACEFUL SHUTDOWN
     */
    printf("\n====================================\n");
    printf("      Graceful Shutdown\n");
    printf("====================================\n");

    if (one_shot_timer_fd >= 0)
    {
        epoll_ctl(epoll_fd,
                  EPOLL_CTL_DEL,
                  one_shot_timer_fd,
                  NULL);

        reactor_timer_destroy(one_shot_timer_fd);
    }

    if (periodic_timer_fd >= 0)
    {
        epoll_ctl(epoll_fd,
                  EPOLL_CTL_DEL,
                  periodic_timer_fd,
                  NULL);

        reactor_timer_destroy(periodic_timer_fd);
    }

    if (server_fd >= 0)
    {
        epoll_ctl(epoll_fd,
                  EPOLL_CTL_DEL,
                  server_fd,
                  NULL);

        close(server_fd);
    }

    reactor_destroy(epoll_fd);

    printf("All resources released.\n");
    printf("SigReactor stopped safely.\n");

    return 0;
}
