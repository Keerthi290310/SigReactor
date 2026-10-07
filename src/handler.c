#include "handler.h"

#include <stdio.h>

static process_handler_t process_handler = NULL;

void reactor_set_process_handler(process_handler_t handler)
{
    process_handler = handler;
}

void reactor_report_process_status(pid_t pid, int status)
{
    if (process_handler != NULL) {
        process_handler(pid, status);
        return;
    }

    printf("Process %ld completed with status %d\n",
           (long)pid,
           status);
}
