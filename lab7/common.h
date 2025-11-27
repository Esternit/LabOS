#ifndef COMMON_H
#define COMMON_H

#define SHM_NAME "/my_shm_time"
#define LOCK_FILE "/tmp/sender.lock"
#define BUF_SIZE 256

typedef struct
{
    char data[BUF_SIZE];
    pid_t sender_pid;
} SharedData;

#endif