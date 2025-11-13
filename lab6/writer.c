#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define FIFO_NAME "/tmp/example_fifo"

int main() {
    mkfifo(FIFO_NAME, 0666);

    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open writer");
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    char message[256];
    snprintf(message, sizeof(message),
             "Time: %sPID: %d\n", ctime(&now), (int)getpid());

    if (write(fd, message, strlen(message)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    printf("[Writer] Sent message.\n");
    return 0;
}