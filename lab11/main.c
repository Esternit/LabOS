#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define NUM_READERS 10
#define ARRAY_SIZE 32
#define MAX_RECORDS 10

char shared_array[ARRAY_SIZE];
int current_version = 0;
int readers_done_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_updated = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_readers_done = PTHREAD_COND_INITIALIZER;

void my_usleep(useconds_t usec) {
    struct timespec ts;
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

void* reader_thread(void* arg) {
    long id = (long)arg;
    int last_seen_version = 0;

    while (last_seen_version < MAX_RECORDS) {
        pthread_mutex_lock(&mutex);

        while (current_version <= last_seen_version && current_version < MAX_RECORDS) {
            pthread_cond_wait(&cond_updated, &mutex);
        }


        if (current_version > MAX_RECORDS) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        char local_copy[ARRAY_SIZE];
        strncpy(local_copy, shared_array, ARRAY_SIZE - 1);
        local_copy[ARRAY_SIZE - 1] = '\0';

        last_seen_version = current_version;
        readers_done_count++;

        if (readers_done_count >= NUM_READERS) {
            pthread_cond_signal(&cond_readers_done);
        }

        pthread_mutex_unlock(&mutex);

        printf("Reader %ld: %s\n", id, local_copy);

        my_usleep(50000);
    }

    return NULL;
}

void* writer_thread(void* arg) {
    (void)arg;

    pthread_mutex_lock(&mutex);
    strncpy(shared_array, "Initial state", ARRAY_SIZE - 1);
    shared_array[ARRAY_SIZE - 1] = '\0';
    current_version = 0;
    pthread_mutex_unlock(&mutex);

    my_usleep(100000);

    for (int i = 1; i <= MAX_RECORDS; ++i) {
        pthread_mutex_lock(&mutex);

        snprintf(shared_array, ARRAY_SIZE, "Record #%d", i);
        current_version = i;
        readers_done_count = 0;

        pthread_cond_broadcast(&cond_updated);

        while (readers_done_count < NUM_READERS) {
            pthread_cond_wait(&cond_readers_done, &mutex);
        }

        pthread_mutex_unlock(&mutex);
        my_usleep(200000);
    }

    pthread_mutex_lock(&mutex);
    current_version = MAX_RECORDS + 1;
    pthread_cond_broadcast(&cond_updated);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    pthread_t readers[NUM_READERS];
    pthread_t writer;

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
    
    return 0;
}