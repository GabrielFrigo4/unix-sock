#ifndef SERVER_H
#define SERVER_H

void handle_request(int client_socket);
void serve_file(int client_socket, const char *filepath);

#endif
