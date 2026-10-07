#define _GNU_SOURCE

#include "timer.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>

/*
 * Create a timerfd.
 *
 * milliseconds:
 *     Timer duration in milliseconds.
 *
 * periodic:
 *     0 -> one-shot timer
 *     1 -> periodic timer
 *
 * Returns:
 *     Timer file descriptor on success
 *     -1 on failure
 */
int reactor_timer_create(int milliseconds, int periodic)
{
    int timer_fd;

    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

    if (timer_fd == -1)
        return -1;

    struct itimerspec timer = {0};

    /* Initial expiration */
    timer.it_value.tv_sec = milliseconds / 1000;
    timer.it_value.tv_nsec =
        (milliseconds % 1000) * 1000000L;

    /* Periodic interval */
    if (periodic) {
        timer.it_interval.tv_sec = milliseconds / 1000;
        timer.it_interval.tv_nsec =
            (milliseconds % 1000) * 1000000L;
    }

    if (timerfd_settime(timer_fd, 0, &timer, NULL) == -1) {
        close(timer_fd);
        return -1;
    }

    return timer_fd;
}


/*
 * Read timer expiration count.
 *
 * The number of expirations is returned through
 * the 'expirations' pointer.
 *
 * Returns:
 *     0  -> success
 *    -1  -> failure
 */
int reactor_timer_read(int timer_fd, uint64_t *expirations)
{
    ssize_t n;

    if (expirations == NULL)
        return -1;

    n = read(timer_fd, expirations, sizeof(*expirations));

    if (n != sizeof(*expirations))
        return -1;

    return 0;
}


/*
 * Destroy/close the timer.
 */
void reactor_timer_destroy(int timer_fd)
{
    if (timer_fd >= 0)
        close(timer_fd);
}
