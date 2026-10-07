#include "timer.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    printf("M7: creating 1-second periodic timer...\n");
    fflush(stdout);

    int timer_fd = reactor_timer_create(1000, 1);

    if (timer_fd < 0) {
        perror("reactor_timer_create");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        uint64_t expirations = 0;

        if (reactor_timer_read(timer_fd, &expirations) != 0) {
            perror("reactor_timer_read");
            reactor_timer_destroy(timer_fd);
            return 1;
        }

        printf("M7: timer event received (%llu expiration(s))\n",
               (unsigned long long)expirations);
        fflush(stdout);
    }

    reactor_timer_destroy(timer_fd);

    printf("M7: timer test completed successfully\n");

    return 0;
}
