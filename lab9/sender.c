#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_NAME "/my_shm_time"
#define SEM_NAME  "/my_sem_sync"
#define BUF_SIZE  256

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Sender already running! Exiting.\n");
            exit(EXIT_FAILURE);
        }
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, BUF_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    char* shared_buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_buf == MAP_FAILED) {
        perror("mmap sender");
        exit(EXIT_FAILURE);
    }

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open sender");
        exit(EXIT_FAILURE);
    }

    printf("Sender PID: %d\n", (int)getpid());
    while (1) {
        time_t now = time(NULL);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

        sem_wait(sem);
        snprintf(shared_buf, BUF_SIZE, "Time: %s | Sender PID: %d", time_str, (int)getpid());
        sem_post(sem);

        sleep(3);
    }

    munmap(shared_buf, BUF_SIZE);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    return 0;
}