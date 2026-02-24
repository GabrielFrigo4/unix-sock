#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../inc/boot.h"

int iniciar_servidor(int porta) {
    int serv_file_desc;
    
    // 1. Criar socket
    if ((serv_file_desc = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    printf("\nSocket criado: %d\n", serv_file_desc);

    struct sockaddr_in6 ender_serv;
    memset(&ender_serv, 0, sizeof(ender_serv));
    ender_serv.sin6_family = AF_INET6;
    ender_serv.sin6_addr = in6addr_any;
    ender_serv.sin6_port = htons(porta);
    
    // 2. Bind
    if (bind(serv_file_desc, (struct sockaddr *)&ender_serv, sizeof(ender_serv)) < 0) {
        perror("Erro no bind");
        close(serv_file_desc);
        exit(EXIT_FAILURE);
    }
    printf("Bind feito com sucesso!\n");

    // 3. Listen
    if (listen(serv_file_desc, 3) < 0) {
        perror("Erro no listen");
        close(serv_file_desc);
        exit(EXIT_FAILURE);
    }
    printf("Servidor iniciado. Aguardando conexão na porta %d...\n\n", porta);

    return serv_file_desc;
}