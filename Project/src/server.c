#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server.h"

#define STATIC_DIR "./web/"

static void send_http_response(int socket, const char *status, const char *content_type, const char *corpo)
{
    char resposta[1024];
    snprintf(resposta, sizeof(resposta),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, content_type, strlen(corpo), corpo);

    send(socket, resposta, strlen(resposta), 0);
}

void handle_request(int novo_socket)
{
    char metodo[10] = {0}, caminho[256] = {0}, versao[20] = {0};
    char buffer[2048] = {0};

    int bytes_recebidos = recv(novo_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_recebidos <= 0)
    {
        if (bytes_recebidos < 0)
            perror("[ERR]: Error reading from client");
        else
            printf("[WARN]: Client disconnected.\n");
        return;
    }

    buffer[bytes_recebidos] = '\0';
    sscanf(buffer, "%9s %255s %19s", metodo, caminho, versao);
    printf("[INFO]: Method: %s | Path: %s\n", metodo, caminho);

    if (strstr(caminho, "..") != NULL)
    {
        send_http_response(novo_socket, "403 Forbidden", "text/plain", "Access Denied.");
        return;
    }

    char *corpo = strstr(buffer, "\r\n\r\n");
    if (corpo)
        corpo += 4;

    if (strcmp(metodo, "GET") == 0)
    {
        char arquivo_alvo[512];
        snprintf(arquivo_alvo, sizeof(arquivo_alvo), "%s%s", STATIC_DIR, strcmp(caminho, "/") == 0 ? "index.html" : caminho + 1);

        serve_file(novo_socket, arquivo_alvo);
    }
    else if (strcmp(metodo, "POST") == 0)
    {
        if (corpo && strlen(corpo) > 0)
        {
            printf("[INFO]: Data received from client: %s\n", corpo);
        }

        char *json = "{\"status\": \"success\", \"message\": \"POST received by C server!\"}";
        send_http_response(novo_socket, "200 OK", "application/json", json);
        printf("[INFO]: POST response sent successfully.\n");
    }
    else
    {
        send_http_response(novo_socket, "405 Method Not Allowed", "text/plain", "Method not supported.");
    }
}

void serve_file(int socket_cliente, const char *arquivo)
{
    FILE *f = fopen(arquivo, "rb");
    if (f == NULL)
    {
        perror("[ERR]: Error opening file");
        send_http_response(socket_cliente, "404 Not Found", "text/html", "<h1>404 - File not found</h1>");
        return;
    }

    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *conteudo = (char *)malloc(tamanho);
    if (conteudo == NULL)
    {
        perror("[ERR]: Memory allocation error");
        fclose(f);
        send_http_response(socket_cliente, "500 Internal Server Error", "text/plain", "Internal memory error.");
        return;
    }

    fread(conteudo, 1, tamanho, f);

    char cabecalho[512];
    snprintf(cabecalho, sizeof(cabecalho),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             tamanho);

    send(socket_cliente, cabecalho, strlen(cabecalho), 0);
    send(socket_cliente, conteudo, tamanho, 0);

    free(conteudo);
    fclose(f);
}
