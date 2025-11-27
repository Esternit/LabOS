#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define DATA_FILE "/tmp/time_data.txt"

int main()
{
    pid_t my_pid = getpid();
    printf("Receiver (PID: %d) reading from %s...\n", my_pid, DATA_FILE);

    char last_data[256] = {0};

    while (1)
    {
        FILE *f = fopen(DATA_FILE, "r");
        if (!f)
        {
            sleep(1);
            continue;
        }

        char current_data[256];
        if (fgets(current_data, sizeof(current_data), f) != NULL)
        {
            current_data[strcspn(current_data, "\n")] = 0;

            if (strcmp(current_data, last_data) != 0)
            {
                strcpy(last_data, current_data);

                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                struct tm tm_info;
                localtime_r(&ts.tv_sec, &tm_info);
                char my_time[64];
                strftime(my_time, sizeof(my_time), "%Y-%m-%d %H:%M:%S", &tm_info);

                printf("[Receiver PID:%d | Time:%s] Received: %s\n", my_pid, my_time, current_data);
            }
        }
        fclose(f);
        sleep(1);
    }
    return 0;
}