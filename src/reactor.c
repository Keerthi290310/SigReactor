#define _GNU_SOURCE

#include "reactor.h"
#include "signal_handler.h"

#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

int reactor_create(void)
{
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return -1;
    }

    return epoll_fd;
}

int reactor_add_mode(int epoll_fd,
                     int fd,
                     int edge_triggered)
{
    if (epoll_fd < 0 || fd < 0)
        return -1;

    struct epoll_event event;

    event.events = EPOLLIN;

    if (edge_triggered)
        event.events |= EPOLLET;

    event.data.fd = fd;

    if (epoll_ctl(epoll_fd,
                  EPOLL_CTL_ADD,
                  fd,
                  &event) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    return 0;
}

int reactor_add(int epoll_fd, int fd)
{
    /*
     * Default mode is Level Triggered.
     * Existing applications remain compatible.
     */
    return reactor_add_mode(epoll_fd,
                            fd,
                            REACTOR_LEVEL_TRIGGERED);
}

int reactor_add_signal(int epoll_fd)
{
    if (epoll_fd < 0)
        return -1;

    if (reactor_signal_init() == -1)
        return -1;

    int signal_fd = reactor_signal_fd();

    if (signal_fd < 0)
    {
        reactor_signal_cleanup();
        return -1;
    }

    if (reactor_add(epoll_fd, signal_fd) == -1)
    {
        reactor_signal_cleanup();
        return -1;
    }

    return signal_fd;
}

int reactor_get_signal_fd(void)
{
    return reactor_signal_fd();
}

int reactor_read_signal(int *signal_number)
{
    if (signal_number == NULL)
        return -1;

    return reactor_signal_read(signal_number);
}

void reactor_destroy(int epoll_fd)
{
    reactor_signal_cleanup();

    if (epoll_fd >= 0)
        close(epoll_fd);
}
