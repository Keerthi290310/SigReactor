#ifndef REACTOR_H
#define REACTOR_H

#include <sys/types.h>

/*
 * Create the epoll instance.
 */
int reactor_create(void);

/*
 * Add a file descriptor to the epoll instance.
 */
int reactor_add(int epoll_fd, int fd);

/*
 * Register SIGINT and SIGTERM using signalfd.
 */
int reactor_add_signal(int epoll_fd);

/*
 * Get the signal file descriptor.
 */
int reactor_get_signal_fd(void);

/*
 * Read a pending signal.
 */
int reactor_read_signal(int *signal_number);

/*
 * Close the reactor epoll instance.
 */
void reactor_destroy(int epoll_fd);

#endif
