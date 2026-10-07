#define _GNU_SOURCE

#include "signal_handler.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

static int signal_fd = -1;
static sigset_t signal_mask;

int reactor_signal_init(void)
{
    sigemptyset(&signal_mask);

    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &signal_mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }

    signal_fd = signalfd(
        -1,
        &signal_mask,
        SFD_CLOEXEC | SFD_NONBLOCK
    );

    if (signal_fd == -1) {
        perror("signalfd");
        return -1;
    }

    return 0;
}

int reactor_signal_fd(void)
{
    return signal_fd;
}

int reactor_signal_read(int *signal_number)
{
    struct signalfd_siginfo info;

    ssize_t n = read(
        signal_fd,
        &info,
        sizeof(info)
    );

    if (n == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            return 0;
        }

        perror("read(signalfd)");
        return -1;
    }

    if (n != sizeof(info)) {
        fprintf(stderr, "Incomplete signal information\n");
        return -1;
    }

    *signal_number = (int)info.ssi_signo;

    return 1;
}

void reactor_signal_cleanup(void)
{
    if (signal_fd != -1) {
        close(signal_fd);
        signal_fd = -1;
    }
}
