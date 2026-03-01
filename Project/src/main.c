#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include "main.h"
#include "server.h"
#include "http.h"

int main()
{
    signal(SIGCHLD, SIG_IGN);
    int server_socket = server_init(PORT);

    int client_socket;
    struct sockaddr_in cliente;
    socklen_t c = sizeof(cliente);
    while ((client_socket = accept(server_socket, (struct sockaddr *)&cliente, &c)))
    {
        if (client_socket < 0)
        {
            perror("[ERR]: Error in accept");
            continue;
        }
        printf("[INFO]: New socket created: %d\n", client_socket);

        switch (fork())
        {
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
            printf("[INFO]: New socket closed.\n\n");
            break;
        }
    }

    close(server_socket);
    printf("[INFO]: Server terminated successfully!\n");
    return EXIT_SUCCESS;
}

