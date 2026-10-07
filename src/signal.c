#define _GNU_SOURCE

#include "signal_handler.h"

#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

static int signal_fd = -1;
static sigset_t signal_mask;

int reactor_signal_init(void)
{
    sigemptyset(&signal_mask);

    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGTERM);
    sigaddset(&signal_mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &signal_mask, NULL) == -1)
        return -1;

    signal_fd = signalfd(-1, &signal_mask, SFD_CLOEXEC);

    if (signal_fd == -1)
        return -1;

    return 0;
}

int reactor_signal_fd(void)
{
    return signal_fd;
}

int reactor_signal_read(int *signal_number)
{
    struct signalfd_siginfo info;
    ssize_t n;

    if (signal_number == NULL)
        return -1;

    n = read(signal_fd, &info, sizeof(info));

    if (n != sizeof(info))
        return -1;

    *signal_number = (int)info.ssi_signo;

    return 0;
}

void reactor_signal_cleanup(void)
{
    if (signal_fd >= 0) {
        close(signal_fd);
        signal_fd = -1;
    }

    sigemptyset(&signal_mask);
}
