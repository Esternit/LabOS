#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void atexit_handler(void)
{
    printf("[atexit] Process %d is exiting.\n", (int)getpid());
}

void sigint_handler(int sig)
{
    printf("Process %d received SIGINT (signal %d). Exiting.\n", (int)getpid(), sig);
    exit(0);
}

void sigterm_handler(int sig)
{
    printf("Process %d received SIGTERM (signal %d). Exiting.\n", (int)getpid(), sig);
    exit(0);
}

int main(void)
{
    pid_t pid;
    int status;

    if (atexit(atexit_handler) != 0) {
        perror("atexit failed");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal(SIGINT) failed");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM) failed");
        exit(EXIT_FAILURE);
    }

    printf("Parent process starting. PID = %d, PPID = %d\n", (int)getpid(), (int)getppid());

    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        printf("Child process: PID = %d, PPID = %d\n", (int)getpid(), (int)getppid());
        sleep(2);
        printf("Child process exiting.\n");
        exit(42);
    }
    else {
        printf("Parent: created child with PID = %d\n", (int)pid);

        if (wait(&status) == -1) {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("Parent: child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent: child terminated by signal %d\n", WTERMSIG(status));
        } else {
            printf("Parent: child stopped or continued\n");
        }

        printf("Parent process exiting.\n");
    }

    return 0;
}