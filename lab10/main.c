#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define NUM_READERS 10
#define ARRAY_SIZE 32

char shared_array[ARRAY_SIZE];
volatile int writer_counter = 0;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void my_usleep(useconds_t usec) {
    struct timespec ts;
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

void* reader_thread(void* arg) {
    long id = (long)arg;
    char local_copy[ARRAY_SIZE];

    for (int i = 0; i < 5; ++i) {
        pthread_rwlock_rdlock(&rwlock);
        strncpy(local_copy, shared_array, ARRAY_SIZE - 1);
        local_copy[ARRAY_SIZE - 1] = '\0';
        pthread_rwlock_unlock(&rwlock);

        printf("Reader %ld: %s\n", id, local_copy);
        my_usleep(100000);
    }

    return NULL;
}

void* writer_thread(void* arg) {
    (void)arg;
    for (int i = 1; i <= 10; ++i) {
        pthread_rwlock_wrlock(&rwlock);
        snprintf(shared_array, ARRAY_SIZE, "Record #%d", i);
        writer_counter = i;
        pthread_rwlock_unlock(&rwlock);

        my_usleep(200000);
    }
    return NULL;
}

int main() {
    pthread_t readers[NUM_READERS];
    pthread_t writer;

    strncpy(shared_array, "Initial state", ARRAY_SIZE - 1);
    shared_array[ARRAY_SIZE - 1] = '\0';

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

    for (int i = 0; i < NUM_READERS; ++i) {
        pthread_join(readers[i], NULL);
    }
    pthread_join(writer, NULL);

    pthread_rwlock_destroy(&rwlock);

    return 0;
}