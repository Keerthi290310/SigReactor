#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

int reactor_signal_init(void);

int reactor_signal_fd(void);

int reactor_signal_read(int *signal_number);

void reactor_signal_cleanup(void);

#endif
