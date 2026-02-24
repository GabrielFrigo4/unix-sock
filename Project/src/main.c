#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Importando nossos módulos
#include "../inc/main.h"
#include "../inc/boot.h"
#include "../inc/server.h"

int main() {
    // Inicia o servidor e pega o descritor
    int serv_file_desc = iniciar_servidor(PORTA);

    while (1) {
        struct sockaddr_in6 cliente;
        socklen_t c = sizeof(cliente);
        int novo_socket = accept(serv_file_desc, (struct sockaddr *)&cliente, &c);
        
        if (novo_socket < 0) {
            perror("Erro no accept");
            continue; 
        }
        printf("Novo socket criado: %d\n", novo_socket);

        // Processa o HTTP
        processar_requisicao(novo_socket);

        close(novo_socket);
        printf("Novo socket encerrado.\n\n");
    }

    close(serv_file_desc);
    printf("Servidor encerrado com sucesso!\n");
    return EXIT_SUCCESS;
}