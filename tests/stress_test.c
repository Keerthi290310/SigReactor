#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define CLIENT_COUNT 20

int main(void)
{
    printf("====================================\n");
    printf("     SigReactor Stress Test\n");
    printf("====================================\n");
    printf("Clients to launch: %d\n\n", CLIENT_COUNT);

    int successful = 0;

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");
            continue;
        }

        if (pid == 0)
        {
            execl("./sigreactor_client",
                  "sigreactor_client",
                  (char *)NULL);

            perror("execl");
            _exit(1);
        }
    }

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        int status;

        pid_t result = wait(&status);

        if (result > 0 &&
            WIFEXITED(status) &&
            WEXITSTATUS(status) == 0)
        {
            successful++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("\nStress Test Results\n");
    printf("-------------------\n");
    printf("Total clients : %d\n", CLIENT_COUNT);
    printf("Successful    : %d\n", successful);
    printf("Failed        : %d\n",
           CLIENT_COUNT - successful);
    printf("Elapsed time  : %.3f seconds\n",
           elapsed);

    if (successful == CLIENT_COUNT)
    {
        printf("\nStress test completed successfully.\n");
        return 0;
    }

    printf("\nStress test completed with failures.\n");
    return 1;
}
