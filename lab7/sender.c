#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "common.h"

static int lock_fd = -1;

void cleanup(int sig)
{
    (void)sig;
    if (lock_fd != -1)
    {
        close(lock_fd);
        unlink(LOCK_FILE);
    }
    shm_unlink(SHM_NAME);
    exit(EXIT_SUCCESS);
}

int main()
{
    lock_fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_RDWR, 0644);
    if (lock_fd == -1)
    {
        if (errno == EEXIST)
        {
            fprintf(stderr, "Sender is already running!\n");
        }
        else
        {
            perror("open lock file");
        }
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1)
    {
        perror("ftruncate");
        close(shm_fd);
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    SharedData *shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    shared->sender_pid = getpid();
    printf("Sender (PID: %d) started.\n", shared->sender_pid);

    while (1)
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm_info;
        localtime_r(&ts.tv_sec, &tm_info);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

        snprintf(shared->data, BUF_SIZE, "PID:%d TIME:%s", shared->sender_pid, time_str);
        sleep(2);
    }
}