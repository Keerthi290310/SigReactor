#define _DEFAULT_SOURCE
#include "process.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *args[] = {
        "/bin/sleep",
        "1",
        NULL
    };

    printf("M4: starting child process...\n");

    pid_t pid = reactor_spawn("/bin/sleep", args);

    if (pid < 0) {
        return 1;
    }

    printf("M4: child PID = %ld\n", (long)pid);

    while (1) {
        int reaped = reactor_reap_children();

        if (reaped > 0) {
            printf("M4: child successfully reaped\n");
            break;
        }

        usleep(100000);
    }

    printf("M4: process integration test completed\n");

    return 0;
}
