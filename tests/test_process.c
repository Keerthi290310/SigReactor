#include "process.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *args[] = {
        "/bin/echo",
        "SigReactor M4: child process executed successfully",
        NULL
    };

    pid_t pid = reactor_spawn("/bin/echo", args);

    if (pid < 0) {
        return 1;
    }

    printf("Parent: created child process with PID %ld\n", (long)pid);

    sleep(1);

    int reaped = reactor_reap_children();

    printf("Parent: reaped %d child process(es)\n", reaped);

    return 0;
}
