#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server.h"
#include "http.h"

#define STATIC_DIR "./web/"
#define CHUNK_SIZE 8192

static const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext)
        return "application/octet-stream";

    if (strcmp(ext, ".html") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".json") == 0)
        return "application/json";

    return "application/octet-stream";
}

static http_method_t parse_method(const char *method_str) {
    if (strcmp(method_str, "GET") == 0)
        return HTTP_GET;
    if (strcmp(method_str, "POST") == 0)
        return HTTP_POST;
    if (strcmp(method_str, "PUT") == 0)
        return HTTP_PUT;
    if (strcmp(method_str, "PATCH") == 0)
        return HTTP_PATCH;
    if (strcmp(method_str, "DELETE") == 0)
        return HTTP_DELETE;
    return HTTP_UNKNOWN;
}

static void parse_http_request(char *buffer, http_request_t *req) {
    req->header_count = 0;
    req->body = NULL;

    char *body_sep = strstr(buffer, "\r\n\r\n");
    if (body_sep) {
        *body_sep = '\0';
        req->body = body_sep + 4;
    }

    char *saveptr;
    char *line = strtok_r(buffer, "\r\n", &saveptr);
    if (!line)
        return;

    char method_str[10] = {0};
    char path_str[256] = {0};
    char version_str[20] = {0};

    sscanf(line, "%9s %255s %19s", method_str, path_str, version_str);
    req->method = parse_method(method_str);
    req->path = path_str;
    req->version = version_str;

    while ((line = strtok_r(NULL, "\r\n", &saveptr)) && req->header_count < 32) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            req->headers[req->header_count].key = line;
            req->headers[req->header_count].value = colon + 1;

            while (*(req->headers[req->header_count].value) == ' ')
                req->headers[req->header_count].value++;

            req->header_count++;
        }
    }
}

static void send_http_response(int client_socket, http_response_t *res) {
    char header_buffer[1024];
    int offset = snprintf(header_buffer,
                          sizeof(header_buffer),
                          "HTTP/1.1 %d %s\r\n"
                          "Content-Type: %s\r\n"
                          "Connection: close\r\n",
                          res->status_code,
                          res->status_message,
                          res->content_type);

    for (int i = 0; i < res->header_count; i++) {
        offset += snprintf(header_buffer + offset,
                           sizeof(header_buffer) - offset,
                           "%s: %s\r\n",
                           res->headers[i].key,
                           res->headers[i].value);
    }

    if (res->file_path) {
        FILE *file = fopen(res->file_path, "rb");
        if (!file) {
            res->status_code = 404;
            res->status_message = "Not Found";
            res->content_type = "text/html";
            res->body = "<h1>404 - File not found</h1>";
            res->body_len = strlen(res->body);
            res->file_path = NULL;
            send_http_response(client_socket, res);
            return;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        snprintf(header_buffer + offset, sizeof(header_buffer) - offset, "Content-Length: %ld\r\n\r\n", file_size);
        send(client_socket, header_buffer, strlen(header_buffer), 0);

        char chunk[CHUNK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(chunk, 1, sizeof(chunk), file)) > 0) {
            if (send(client_socket, chunk, bytes_read, 0) < 0)
                break;
        }
        fclose(file);
    } else {
        snprintf(header_buffer + offset,
                 sizeof(header_buffer) - offset,
                 "Content-Length: %lu\r\n\r\n",
                 (unsigned long)res->body_len);
        send(client_socket, header_buffer, strlen(header_buffer), 0);

        if (res->body && res->body_len > 0) {
            send(client_socket, res->body, res->body_len, 0);
        }
    }
}

static void handle_get_request(int client_socket, http_request_t *req) {
    char target_file[512];
    snprintf(target_file,
             sizeof(target_file),
             "%s%s",
             STATIC_DIR,
             strcmp(req->path, "/") == 0 ? "index.html" : req->path + 1);

    http_response_t res = {0};
    res.status_code = 200;
    res.status_message = "OK";
    res.content_type = get_mime_type(target_file);
    res.file_path = target_file;

    send_http_response(client_socket, &res);
}

static void handle_post_request(int client_socket, http_request_t *req) {
    if (req->body && strlen(req->body) > 0) {
        printf("[INFO]: Data received: %s\n", req->body);
    }

    http_response_t res = {0};
    res.status_code = 200;
    res.status_message = "OK";
    res.content_type = "application/json";
    res.body = "{\"status\": \"success\", \"message\": \"POST received via struct!\"}";
    res.body_len = strlen(res.body);

    send_http_response(client_socket, &res);
}

void http_handle_client(int client_socket) {
    char buffer[4096] = {0};
    int total_received = 0;
    int bytes_received;

    while (total_received < (int)(sizeof(buffer) - 1)) {
        bytes_received = recv(client_socket, buffer + total_received, sizeof(buffer) - 1 - total_received, 0);
        if (bytes_received <= 0)
            break;

        total_received += bytes_received;
        buffer[total_received] = '\0';

        if (strstr(buffer, "\r\n\r\n") != NULL)
            break;
    }

    if (total_received == 0)
        return;

    http_request_t req;
    parse_http_request(buffer, &req);

    printf("[INFO]: Method Code: %d | Path: %s\n", req.method, req.path);

    if (strstr(req.path, "..") != NULL) {
        http_response_t res = {0};
        res.status_code = 403;
        res.status_message = "Forbidden";
        res.content_type = "text/plain";
        res.body = "Access Denied.";
        res.body_len = strlen(res.body);
        send_http_response(client_socket, &res);
        return;
    }

    switch (req.method) {
    case HTTP_GET:
        handle_get_request(client_socket, &req);
        break;
    case HTTP_POST:
        handle_post_request(client_socket, &req);
        break;
    default: {
        http_response_t res = {0};
        res.status_code = 405;
        res.status_message = "Method Not Allowed";
        res.content_type = "text/plain";
        res.body = "Method not supported.";
        res.body_len = strlen(res.body);
        send_http_response(client_socket, &res);
        break;
    }
    }
}
