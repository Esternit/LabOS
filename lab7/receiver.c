#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#include "common.h"

static volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

int main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open (receiver) — sender may not be running");
        exit(EXIT_FAILURE);
    }

    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED)
    {
        perror("mmap (receiver)");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    pid_t my_pid = getpid();
    char last_data[BUF_SIZE] = {0};

    printf("Receiver (PID: %d) started, waiting for data...\n", my_pid);

    while (keep_running)
    {
        if (strlen(shared->data) > 0 && strcmp(shared->data, last_data) != 0)
        {
            strncpy(last_data, shared->data, BUF_SIZE - 1);
            last_data[BUF_SIZE - 1] = '\0';

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            struct tm tm_info;
            localtime_r(&ts.tv_sec, &tm_info);
            char my_time[64];
            strftime(my_time, sizeof(my_time), "%Y-%m-%d %H:%M:%S", &tm_info);

            printf("[Receiver PID:%d | Time:%s] Received: %s\n", my_pid, my_time, last_data);
        }
        sleep(1);
    }

    printf("Receiver shutting down...\n");

    if (munmap(shared, sizeof(SharedData)) == -1)
        perror("munmap");

    if (close(shm_fd) == -1)
        perror("close shm_fd");

    return EXIT_SUCCESS;
}