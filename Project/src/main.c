#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "main.h"
#include "server.h"
#include "http.h"

static void sigchld_handler(int s) {
    int saved_errno = errno;
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[INFO]: Zombie reaped (PID: %d) | Exit Status: %d\n", pid, WEXITSTATUS(status));
    }

    errno = saved_errno;
}

static void sigchld_setup() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("[ERR]: Error configuring sigaction");
        exit(EXIT_FAILURE);
    }
}

int main() {
    sigchld_setup();

    int server_socket = server_init(PORT);
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len))) {
        if (client_socket < 0) {
            perror("[ERR]: Error in accept");
            continue;
        }
        printf("[INFO]: New socket created: %d\n", client_socket);

        switch (fork()) {
        case -1:
            close(client_socket);
            perror("[ERR]: Error creating child process (fork)");
            break;
        case 0:
            close(server_socket);
            printf("[INFO]: Child process (PID: %d) started handling socket %d.\n", getpid(), client_socket);
            http_handle_client(client_socket);
            close(client_socket);
            printf("[INFO]: Child process (PID: %d) closed socket %d and is exiting.\n", getpid(), client_socket);
            exit(EXIT_SUCCESS);
            break;
        default:
            close(client_socket);
            break;
        }
    }

    close(server_socket);
    printf("[INFO]: Server terminated successfully!\n");
    return EXIT_SUCCESS;
}
