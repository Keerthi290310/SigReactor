#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

typedef struct {
    pid_t pid;
    int status;
} reactor_child_t;

/* Start a child process */
pid_t reactor_spawn(const char *program, char *const argv[]);

/* Check and reap exited children */
int reactor_reap_children(void);

#endif
