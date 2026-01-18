#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#define SHM_SIZE 256
#define KEY_FILE "ipc92.key"

static int shmid = -1;
static int semid = -1;
static char *shared_buf = NULL;
static volatile int stop = 0;

int sem_lock(int semid) {
    struct sembuf sb = {0, -1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

int sem_unlock(int semid) {
    struct sembuf sb = {0, 1, SEM_UNDO};
    return semop(semid, &sb, 1);
}

void cleanup(int sig) {
    (void)sig;
    stop = 1;
    if (shared_buf && shared_buf != (void*)-1) {
        shmdt(shared_buf);
    }
    unlink(KEY_FILE);
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    while (access(KEY_FILE, F_OK) == -1) {
        sleep(1);
    }

    key_t shm_key = ftok(KEY_FILE, 'M');
    key_t sem_key = ftok(KEY_FILE, 'S');
    if (shm_key == -1 || sem_key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    shmid = shmget(shm_key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget (receiver)");
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, 0666);
    if (semid == -1) {
        perror("semget (receiver)");
        exit(EXIT_FAILURE);
    }

    shared_buf = (char*)shmat(shmid, NULL, 0);
    if (shared_buf == (void*)-1) {
        perror("shmat (receiver)");
        exit(EXIT_FAILURE);
    }

    printf("Receiver PID: %d\n", (int)getpid());

    char last_message[SHM_SIZE] = {0};

    while (!stop) {
        if (sem_lock(semid) == -1) {
            perror("receiver sem_lock");
            break;
        }

        if (strcmp(shared_buf, last_message) != 0) {
            printf("Receiver PID: %d | Received: %s\n", (int)getpid(), shared_buf);
            strncpy(last_message, shared_buf, sizeof(last_message) - 1);
            last_message[sizeof(last_message) - 1] = '\0';
        }

        if (sem_unlock(semid) == -1) {
            perror("receiver sem_unlock");
        }

        sleep(1);
    }

    cleanup(0);
    return 0;
}