#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 32

char shared_buffer[BUFFER_SIZE] = "0";
sem_t mutex_sem;

void* writer_thread(void* arg) {
    (void)arg;
    int counter = 1;
    while (counter <= 10) {
        sem_wait(&mutex_sem);
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        sem_post(&mutex_sem);
        sleep(1);
        counter++;
    }
    return NULL;
}

void* reader_thread(void* arg) {
    (void)arg;
    pthread_t tid = pthread_self();
    int i = 0;
    while (i < 12) {
        sem_wait(&mutex_sem);
        printf("Reader TID=%lu: %s\n", (unsigned long)tid, shared_buffer);
        sem_post(&mutex_sem);

        struct timespec ts = { .tv_sec = 0, .tv_nsec = 800000000L };
        nanosleep(&ts, NULL);

        i++;
    }
    return NULL;
}

int main() {
    pthread_t writer, reader;

    if (sem_init(&mutex_sem, 0, 1) != 0) {
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0 ||
        pthread_create(&reader, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    sem_destroy(&mutex_sem);
    return 0;
}