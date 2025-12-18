#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <time.h> 
#include <errno.h>

#define SHM_SIZE 32
#define KEY_FILE "ipc91.key"

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
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    unlink(KEY_FILE);
    exit(EXIT_SUCCESS);
}

void* writer_thread(void* arg) {
    (void)arg;
    int counter = 1;
    while (counter <= 10 && !stop) {
        if (sem_lock(semid) == -1) {
            perror("writer sem_lock");
            break;
        }
        snprintf(shared_buf, SHM_SIZE, "%d", counter);
        if (sem_unlock(semid) == -1) {
            perror("writer sem_unlock");
        }
        sleep(1);
        counter++;
    }
    return NULL;
}

void* reader_thread(void* arg) {
    (void)arg;
    int count = 0;
    while (count < 12 && !stop) {
        if (sem_lock(semid) == -1) {
            perror("reader sem_lock");
            break;
        }
        printf("Reader TID=%lu: %s\n", (unsigned long)pthread_self(), shared_buf);
        if (sem_unlock(semid) == -1) {
            perror("reader sem_unlock");
        }
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 800000000L };
        nanosleep(&ts, NULL);
        count++;
    }
    return NULL;
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    FILE *fp = fopen(KEY_FILE, "w");
    if (!fp) {
        perror("fopen ipc91.key");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    key_t shm_key = ftok(KEY_FILE, 'M');
    key_t sem_key = ftok(KEY_FILE, 'S');
    if (shm_key == -1 || sem_key == -1) {
        perror("ftok");
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }

    shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        perror("shmget");
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        perror("semget");
        shmctl(shmid, IPC_RMID, NULL);
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl SETVAL");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, IPC_RMID, 0);
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }

    shared_buf = (char*)shmat(shmid, NULL, 0);
    if (shared_buf == (void*)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, IPC_RMID, 0);
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }
    strcpy(shared_buf, "0");

    pthread_t writer, reader;
    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0 ||
        pthread_create(&reader, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create");
        cleanup(0);
        exit(EXIT_FAILURE);
    }

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    cleanup(0);
    return 0;
}