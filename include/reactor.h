#ifndef REACTOR_H
#define REACTOR_H

#include <sys/types.h>

#define REACTOR_LEVEL_TRIGGERED 0
#define REACTOR_EDGE_TRIGGERED  1

int reactor_create(void);

int reactor_add(int epoll_fd, int fd);

int reactor_add_mode(int epoll_fd, int fd, int edge_triggered);

int reactor_add_signal(int epoll_fd);

int reactor_get_signal_fd(void);

int reactor_read_signal(int *signal_number);

void reactor_destroy(int epoll_fd);

#endif
