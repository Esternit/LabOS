#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define FIFO_NAME "/tmp/example_fifo"

int main() {
    sleep(10);

    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open reader");
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    printf("[Reader] Current time: %s", ctime(&now));

    char buffer[256];
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[Reader] Received: %s", buffer);
    }

    close(fd);
    unlink(FIFO_NAME);
    return 0;
}