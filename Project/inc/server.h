#ifndef SERVER_H
#define SERVER_H

void processar_requisicao(int novo_socket);
void enviar_arquivo_generico(int socket_cliente, const char *arquivo);

#endif