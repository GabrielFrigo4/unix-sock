#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "boot.h"

int boot_server(int porta) {
    int serv_file_desc;
    struct sockaddr_in ender_serv;
    int opt = 1;

    if ((serv_file_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serv_file_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Erro no setsockopt");
        exit(EXIT_FAILURE);
    }

    printf("\nSocket criado: %d\n", serv_file_desc);

    memset(&ender_serv, 0, sizeof(ender_serv));
    ender_serv.sin_family = AF_INET;
    ender_serv.sin_addr.s_addr = INADDR_ANY;
    ender_serv.sin_port = htons(porta);

    if (bind(serv_file_desc, (struct sockaddr *)&ender_serv, sizeof(ender_serv)) < 0) {
        perror("Erro no bind");
        close(serv_file_desc);
        exit(EXIT_FAILURE);
    }
    printf("Bind feito com sucesso!\n");

    if (listen(serv_file_desc, 10) < 0) {
        perror("Erro no listen");
        close(serv_file_desc);
        exit(EXIT_FAILURE);
    }
    printf("Servidor iniciado. Aguardando conexão na porta %d...\n\n", porta);

    return serv_file_desc;
}
