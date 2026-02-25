#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "main.h"
#include "boot.h"
#include "server.h"

int main() {
    int serv_file_desc = boot_server(PORTA);

    int novo_socket;
    struct sockaddr_in cliente;
    socklen_t c = sizeof(cliente);
    while ((novo_socket = accept(serv_file_desc, (struct sockaddr *)&cliente, &c))) {
        if (novo_socket < 0) {
            perror("Erro no accept");
            continue;
        }
        printf("Novo socket criado: %d\n", novo_socket);

        handle_request(novo_socket);

        close(novo_socket);
        printf("Novo socket encerrado.\n\n");
    }

    close(serv_file_desc);
    printf("Servidor encerrado com sucesso!\n");
    return EXIT_SUCCESS;
}
