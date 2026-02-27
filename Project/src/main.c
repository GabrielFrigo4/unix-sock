#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "main.h"
#include "boot.h"
#include "server.h"

int main()
{
    int server_socket = boot_server(PORT);

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

        handle_request(client_socket);

        close(client_socket);
        printf("[INFO]: New socket closed.\n\n");
    }

    close(server_socket);
    printf("[INFO]: Server terminated successfully!\n");
    return EXIT_SUCCESS;
}
