#include <sys/types.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h>     
#include "../inc/server.h"

// Define a pasta onde os HTMLs estão guardados
#define STATIC_DIR "./stc/"

void processar_requisicao(int novo_socket) {
    char metodo[10], caminho[256], versao[20];
    char buffer[2048] = {0};

    int bytes_recebidos = recv(novo_socket, buffer, sizeof(buffer)-1, 0);
    if (bytes_recebidos < 0) {
        perror("Erro na leitura do cliente");
        close(novo_socket);
        return;
    }
    else if (bytes_recebidos == 0) {
        printf("Cliente desconectou.\n");
        close(novo_socket);
        return;
    }
    else {
        buffer[bytes_recebidos] = '\0';
        printf("Recebidos com êxito: %d bytes.\n", bytes_recebidos);
    }

    sscanf(buffer, "%9s %255s %19s", metodo, caminho, versao);
    printf("O cliente quer usar o método: %s\n", metodo);
    printf("O arquivo solicitado eh: %s\n", caminho);
    
    char *corpo = strstr(buffer, "\r\n\r\n");
    if (corpo) {
        corpo += 4;
        printf("Dados do corpo recebidos: %s\n", corpo);
    }

    if (strcmp(metodo, "GET") == 0) {
        char arquivo_alvo[512];
        memset(arquivo_alvo, 0, sizeof(arquivo_alvo));
        
        // Adiciona o diretório estático antes do nome do arquivo
        strcpy(arquivo_alvo, STATIC_DIR);
        
        if (strcmp(caminho, "/") == 0) { 
            strcat(arquivo_alvo, "index.html");
        }
        else {
            strcat(arquivo_alvo, caminho + 1); 
        }
        enviar_arquivo_generico(novo_socket, arquivo_alvo);
    }
    else if (strcmp(metodo, "POST") == 0) {
        printf("Recebi um POST para o caminho %s\n", caminho);
        
        if (corpo != NULL && strlen(corpo) > 0) {
            printf("Dados recebidos do cliente: %s\n", corpo);
        } else {
            printf("POST recebido, mas o corpo estava vazio.\n");
        }

        char resposta[512];
        memset(resposta, 0, sizeof(resposta));
        char *mensagem_retorno = "{\"status\": \"sucesso\", \"mensagem\": \"POST recebido pelo servidor C!\"}";

        snprintf(resposta, sizeof(resposta),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(mensagem_retorno), mensagem_retorno);

        int bytes_enviados = send(novo_socket, resposta, strlen(resposta), 0);
        if (bytes_enviados < 0) perror("Erro ao enviar resposta do POST");
        else printf("Resposta do POST enviada com sucesso.\n");
    }
}

void enviar_arquivo_generico(int socket_cliente, const char *arquivo) {
    FILE *f = fopen(arquivo, "rb"); 
    if (f == NULL) {
        perror("Erro ao abrir arquivo");
        char *erro = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 - Nao encontrado</h1>";
        send(socket_cliente, erro, strlen(erro), 0);
        return;
    }

    fseek(f, 0, SEEK_END);      
    size_t tamanho = ftell(f);  
    fseek(f, 0, SEEK_SET);      

    char *conteudo = (char*) malloc(tamanho);
    fread(conteudo, 1, tamanho, f); 

    char cabecalho[512]; 
    memset(cabecalho, 0, sizeof(cabecalho));
    
    snprintf(cabecalho, sizeof(cabecalho),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n" 
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n", tamanho);   

    int bytes_cabecalho = send(socket_cliente, cabecalho, strlen(cabecalho), 0);
    if (bytes_cabecalho < 0) {
        perror("Erro ao enviar cabecalho");
        close(socket_cliente);
        free(conteudo);
        fclose(f);
        return;
    }

    int bytes_conteudo = send(socket_cliente, conteudo, tamanho, 0);
    if (bytes_conteudo < 0) {
        perror("Erro ao enviar conteudo");
    }

    free(conteudo);
    fclose(f);    
}