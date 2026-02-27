#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "boot.h"

int boot_server(int port)
{
    int server_socket;
    struct sockaddr_in socket_address;
    int socket_opt = 1;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("[ERR]: Error creating socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)))
    {
        perror("[ERR]: Error in setsockopt");
        exit(EXIT_FAILURE);
    }

    printf("\n[INFO]: Socket created: %d\n", server_socket);

    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("[ERR]: Error in bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("[INFO]: Bind successful!\n");

    if (listen(server_socket, 255) < 0)
    {
        perror("[ERR]: Error in listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("[INFO]: Server started. Waiting for connections on port %d...\n\n", port);

    return server_socket;
}
