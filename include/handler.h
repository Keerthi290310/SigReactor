#ifndef HANDLER_H
#define HANDLER_H

#include <sys/types.h>

typedef void (*process_handler_t)(pid_t pid, int status);

void reactor_set_process_handler(process_handler_t handler);

void reactor_report_process_status(pid_t pid, int status);

#endif
