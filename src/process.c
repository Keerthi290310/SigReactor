#include "process.h"
#include "handler.h"

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t reactor_spawn(const char *program, char *const argv[])
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        execvp(program, argv);

        perror("execvp");
        _exit(127);
    }

    return pid;
}

int reactor_reap_children(void)
{
    int reaped = 0;

    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {
            reaped++;

            reactor_report_process_status(pid, status);
            continue;
        }

        if (pid == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        break;
    }

    return reaped;
}
