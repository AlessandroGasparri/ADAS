#define _XOPEN_SOURCE 700
#include <assert.h>
#include <signal.h>
#include <stdbool.h> /* false */
#include <stdio.h> /* perror */
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <sys/wait.h> /* wait, sleep */
#include <unistd.h> /* fork, write */

void signal_handler(int sig) {
    char s1[] = "SIGUSR1\n";
    char s2[] = "SIGUSR2\n";
    if (sig == SIGUSR1) {
        write(STDOUT_FILENO, s1, sizeof(s1));
    } else if (sig == SIGUSR2) {
        write(STDOUT_FILENO, s2, sizeof(s2));
    }
    signal(sig, signal_handler);
}

int main() {
    pid_t pid;

    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        assert(false);
    } else {
        if (pid == 0) {
            while (1);
            exit(EXIT_SUCCESS);
            }
        while (1) {
        	printf("invio segnalini");
            kill(pid, SIGUSR1);
            sleep(1);
        	printf("invio segnalini");
            kill(pid, SIGUSR2);
            sleep(1);
        }
    }
    return EXIT_SUCCESS;
}