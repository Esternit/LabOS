#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define BUF_SIZE 32

static char shared_buf[BUF_SIZE] = "0";
static sem_t mutex;
static volatile int stop = 0;

void cleanup(int sig) {
    (void)sig;
    stop = 1;
    sem_destroy(&mutex);
    exit(EXIT_SUCCESS);
}

void* writer_thread(void* arg) {
    (void)arg;
    int counter = 1;
    while (counter <= 10 && !stop) {
        sem_wait(&mutex);
        snprintf(shared_buf, BUF_SIZE, "%d", counter);
        sem_post(&mutex);

        sleep(1);
        counter++;
    }
    return NULL;
}

void* reader_thread(void* arg) {
    (void)arg;
    int count = 0;
    while (count < 12 && !stop) {
        sem_wait(&mutex);
        printf("Reader TID=%lu: %s\n", (unsigned long)pthread_self(), shared_buf);
        sem_post(&mutex);

        struct timespec ts = { .tv_sec = 0, .tv_nsec = 800000000L };
        nanosleep(&ts, NULL);
        count++;
    }
    return NULL;
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    if (sem_init(&mutex, 0, 1) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    pthread_t writer, reader;
    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0 ||
        pthread_create(&reader, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create");
        sem_destroy(&mutex);
        exit(EXIT_FAILURE);
    }

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    cleanup(0);
    return 0;
}