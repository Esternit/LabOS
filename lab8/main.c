#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NUM_READERS 10
#define BUFFER_SIZE 32

char shared_buffer[BUFFER_SIZE] = "0";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int writer_done = 0;

void usleep_compat(unsigned long usec) {
    struct timespec ts;
    ts.tv_sec = usec / 1000000UL;
    ts.tv_nsec = (usec % 1000000UL) * 1000UL;
    nanosleep(&ts, NULL);
}

void* writer_thread(void* arg) {
    (void)arg;
    int counter = 1;
    while (counter <= 20) {
        pthread_mutex_lock(&mutex);
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        pthread_mutex_unlock(&mutex);
        usleep_compat(100000);
        counter++;
    }
    writer_done = 1;
    return NULL;
}

void* reader_thread(void* arg) {
    long tid = (long)arg;
    while (!writer_done) {
        pthread_mutex_lock(&mutex);
        printf("Reader %ld: %s\n", tid, shared_buffer);
        pthread_mutex_unlock(&mutex);
        usleep_compat(50000);
    }
    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[NUM_READERS];

    for (long i = 0; i < NUM_READERS; ++i) {
        if (pthread_create(&readers[i], NULL, reader_thread, (void*)i) != 0) {
            perror("Failed to create reader thread");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_create(&writer, NULL, writer_thread, NULL) != 0) {
        perror("Failed to create writer thread");
            exit(EXIT_FAILURE);
    }

    pthread_join(writer, NULL);
    for (int i = 0; i < NUM_READERS; ++i) {
        pthread_join(readers[i], NULL);
    }

    return 0;
}