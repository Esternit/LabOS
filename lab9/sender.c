#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
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
    printf("\nSender stopped.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    FILE *fp = fopen(KEY_FILE, "w");
    if (!fp) {
        perror("fopen ipc92.key");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%d", getpid());
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
        if (errno == EEXIST) {
            fprintf(stderr, "Error: shared memory already exists.\n");
        } else {
            perror("shmget");
        }
        unlink(KEY_FILE);
        exit(EXIT_FAILURE);
    }

    semid = semget(sem_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Error: semaphore already exists.\n");
        } else {
            perror("semget");
        }
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

    printf("Sender PID: %d\n", (int)getpid());

    while (1) {
        if (sem_lock(semid) == -1) {
            perror("sem_lock");
            break;
        }

        time_t now = time(NULL);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
        snprintf(shared_buf, SHM_SIZE, "Time: %s | Sender PID: %d", time_str, (int)getpid());

        if (sem_unlock(semid) == -1) {
            perror("sem_unlock");
        }

        sleep(3);
    }

    cleanup(0);
    return 0;
}