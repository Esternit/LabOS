#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[256];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[1]);

        sleep(5);

        time_t now = time(NULL);
        printf("[Child] Current time: %s", ctime(&now));

        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[Child] Received: %s", buffer);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);

        time_t now = time(NULL);
        char message[256];
        snprintf(message, sizeof(message),
                 "Time: %sPID: %d\n", ctime(&now), (int)getpid());

        write(pipefd[1], message, strlen(message));

        close(pipefd[1]);

        wait(NULL);
    }

    return 0;
}