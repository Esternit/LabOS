#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_NAME "/my_shm_time"
#define SEM_NAME "/my_sem_time"
#define BUF_SIZE 256

int main()
{
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (shm_fd == -1)
    {
        perror("shm_open (receiver)");
        fprintf(stderr, "Error: Sender not running or shared memory not created.\n");
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = mmap(NULL, BUF_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap (receiver)");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED)
    {
        perror("sem_open (receiver)");
        exit(EXIT_FAILURE);
    }

    pid_t my_pid = getpid();
    printf("Receiver started (PID: %d)\n", my_pid);

    while (1)
    {
        if (sem_wait(sem) == -1)
        {
            perror("sem_wait");
            break;
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm_info;
        localtime_r(&ts.tv_sec, &tm_info);
        char my_time[64];
        strftime(my_time, sizeof(my_time), "%Y-%m-%d %H:%M:%S", &tm_info);

        printf("[Receiver PID:%d | Time:%s] Received: %s\n", my_pid, my_time, shm_ptr);
    }

    munmap(shm_ptr, BUF_SIZE);
    close(shm_fd);
    sem_close(sem);
    return 0;
}