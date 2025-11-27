#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

typedef struct
{
    long version;
    char data[200];
} SharedData;

#define SHM_NAME "/my_shm_time"
#define SEM_NAME "/my_sem_time"
#define BUF_SIZE 256

int main()
{
    sem_t *sem_check = sem_open(SEM_NAME "_lock", O_CREAT | O_EXCL, 0644, 1);
    if (sem_check == SEM_FAILED)
    {
        perror("Another sender is already running or semaphore exists");
        fprintf(stderr, "Error: Only one sender process allowed.\n");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, BUF_SIZE) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t my_pid = getpid();
    struct timespec ts;

    printf("Sender started (PID: %d). Sending time every 2 seconds...\n", my_pid);

    SharedData *shared = (SharedData *)shm_ptr;
    long counter = 0;

    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        char buf[64];
        struct tm tm_info;
        localtime_r(&ts.tv_sec, &tm_info);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);

        snprintf(shared->data, sizeof(shared->data), "PID:%d TIME:%s", my_pid, buf);
        __sync_synchronize();
        shared->version = ++counter;

        sem_post(sem);
        sleep(2);
    }

    munmap(shm_ptr, BUF_SIZE);
    close(shm_fd);
    sem_close(sem);
    sem_close(sem_check);
    sem_unlink(SEM_NAME "_lock");

    shm_unlink(SHM_NAME);

    return 0;
}