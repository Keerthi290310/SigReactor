#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Create a timer file descriptor */
int reactor_timer_create(int seconds, int periodic);

/* Read an expired timer */
int reactor_timer_read(int timer_fd, uint64_t *expirations);

/* Close timer */
void reactor_timer_destroy(int timer_fd);

#endif

