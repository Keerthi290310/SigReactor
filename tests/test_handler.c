#include "process.h"
#include "handler.h"

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static void my_process_handler(pid_t pid, int status)
{
    if (WIFEXITED(status)) {
        printf("Handler: child %ld exited with status %d\n",
               (long)pid,
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Handler: child %ld terminated by signal %d\n",
               (long)pid,
               WTERMSIG(status));
    }
}

int main(void)
{
    reactor_set_process_handler(my_process_handler);

    char *args[] = {
        "/bin/echo",
        "M5 handler test successful",
        NULL
    };

    pid_t pid = reactor_spawn("/bin/echo", args);

    if (pid < 0) {
        return 1;
    }

    sleep(1);

    reactor_reap_children();

    return 0;
}
