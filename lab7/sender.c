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
static int shm_fd = -1;
static SharedData *shared = MAP_FAILED;

void cleanup(int sig)
{
    (void)sig;

    if (shared != MAP_FAILED)
    {
        if (munmap(shared, sizeof(SharedData)) == -1)
            perror("munmap in cleanup");
        shared = MAP_FAILED;
    }

    if (shm_fd != -1)
    {
        if (close(shm_fd) == -1)
            perror("close shm_fd in cleanup");
        shm_fd = -1;
    }

    if (lock_fd != -1)
    {
        close(lock_fd);
        unlink(LOCK_FILE);
        lock_fd = -1;
    }

    if (shm_unlink(SHM_NAME) == -1 && errno != ENOENT)
        perror("shm_unlink");

    exit(EXIT_SUCCESS);
}

int main(void)
{
    lock_fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_RDWR, 0644);
    if (lock_fd == -1)
    {
        if (errno == EEXIST)
            fprintf(stderr, "Sender is already running!\n");
        else
            perror("open lock file");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1)
    {
        perror("ftruncate");
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED)
    {
        perror("mmap");
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

    return 0;
}