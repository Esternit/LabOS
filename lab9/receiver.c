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
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open receiver (run sender first!)");
        exit(EXIT_FAILURE);
    }

    char* shared_buf = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_buf == MAP_FAILED) {
        perror("mmap receiver");
        exit(EXIT_FAILURE);
    }

    sem_t* sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open receiver");
        exit(EXIT_FAILURE);
    }

    printf("Receiver PID: %d\n", (int)getpid());
    while (1) {
        sem_wait(sem);
        printf("Receiver PID: %d | Received: %s\n", (int)getpid(), shared_buf);
        sem_post(sem);
        sleep(1);
    }

    munmap(shared_buf, BUF_SIZE);
    sem_close(sem);
    return 0;
}