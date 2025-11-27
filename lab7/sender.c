#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define DATA_FILE "/tmp/time_data.txt"

int main()
{
    pid_t my_pid = getpid();
    printf("Sender (PID: %d) writing to %s every 2 sec...\n", my_pid, DATA_FILE);

    while (1)
    {
        FILE *f = fopen(DATA_FILE, "w");
        if (!f)
        {
            perror("fopen (sender)");
            exit(EXIT_FAILURE);
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm_info;
        localtime_r(&ts.tv_sec, &tm_info);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);

        fprintf(f, "PID:%d TIME:%s\n", my_pid, buf);
        fclose(f);

        sleep(2);
    }
    return 0;
}