#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server.h"

#define STATIC_DIR "./web/"

static void send_http_response(int client_socket, const char *status, const char *content_type, const char *body)
{
    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, content_type, strlen(body), body);

    send(client_socket, response, strlen(response), 0);
}

static void http_send_file(int client_socket, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        perror("[ERR]: Error opening file");
        send_http_response(client_socket, "404 Not Found", "text/html", "<h1>404 - File not found</h1>");
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(size);
    if (!content)
    {
        perror("[ERR]: Memory allocation error");
        fclose(file);
        send_http_response(client_socket, "500 Internal Server Error", "text/plain", "Internal memory error.");
        return;
    }

    fread(content, 1, size, file);

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             size);

    send(client_socket, header, strlen(header), 0);
    send(client_socket, content, size, 0);

    free(content);
    fclose(file);
}

static void handle_get_request(int client_socket, const char *path)
{
    char target_file[512];
    snprintf(target_file, sizeof(target_file), "%s%s", STATIC_DIR, strcmp(path, "/") == 0 ? "index.html" : path + 1);

    http_send_file(client_socket, target_file);
}

static void handle_post_request(int client_socket, const char *body)
{
    if (body && strlen(body) > 0)
    {
        printf("[INFO]: Data received from client: %s\n", body);
    }

    char *json = "{\"status\": \"success\", \"message\": \"POST received by C server!\"}";
    send_http_response(client_socket, "200 OK", "application/json", json);
    printf("[INFO]: POST response sent successfully.\n");
}

void http_handle_client(int client_socket)
{
    char method[10] = {0}, path[256] = {0}, version[20] = {0};
    char buffer[2048] = {0};

    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        if (bytes_received < 0)
            perror("[ERR]: Error reading from client");
        else
            printf("[WARN]: Client disconnected.\n");
        return;
    }

    buffer[bytes_received] = '\0';
    sscanf(buffer, "%9s %255s %19s", method, path, version);
    printf("[INFO]: Method: %s | Path: %s\n", method, path);

    if (strstr(path, "..") != NULL)
    {
        send_http_response(client_socket, "403 Forbidden", "text/plain", "Access Denied.");
        return;
    }

    char *body = strstr(buffer, "\r\n\r\n");
    if (body)
        body += 4;

    if (strcmp(method, "GET") == 0)
    {
        handle_get_request(client_socket, path);
    }
    else if (strcmp(method, "POST") == 0)
    {
        handle_post_request(client_socket, body);
    }
    else
    {
        send_http_response(client_socket, "405 Method Not Allowed", "text/plain", "Method not supported.");
    }
}

