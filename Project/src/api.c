#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api.h"
#include "http.h"

constexpr char DATA_DIR[] = "./data/";
constexpr char API_DATA_PREFIX[] = "/api/data/";
constexpr char API_FILES_ROUTE[] = "/api/files";
constexpr char ROUTE_EDITOR[] = "/editor";
constexpr char ROUTE_EDITOR_HTML[] = "/editor.html";

constexpr char AUTH_CREDENTIALS[] = "Basic YWRtaW46YWRtaW4=";
constexpr char AUTH_REALM[] = "Area Restrita do Admin";

constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t RESP_BUFFER_SIZE = 8192;
constexpr size_t AUTH_HEADER_BUFFER_SIZE = 1024;

static void ensure_data_directory(void)
{
	struct stat st = {};
	if (stat(DATA_DIR, &st) == -1)
	{
		mkdir(DATA_DIR, 0755);
	}
}

static bool requires_auth(const http_request_t *const req)
{
	if (req->method == HTTP_DELETE)
	{
		return true;
	}

	const bool is_editor_route = strcmp(req->path, ROUTE_EDITOR) == 0;
	const bool is_editor_html = strcmp(req->path, ROUTE_EDITOR_HTML) == 0;

	return is_editor_route || is_editor_html;
}

static bool is_authenticated(const http_request_t *const req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcmp(req->headers[i].key, "Authorization") == 0)
		{
			return strcmp(req->headers[i].value, AUTH_CREDENTIALS) == 0;
		}
	}
	return false;
}

static void send_unauthorized(const int client_socket)
{
	const char *const body = "Não autorizado. Credenciais incorretas ou ausentes.";
	char header_buffer[AUTH_HEADER_BUFFER_SIZE];

	snprintf(
	    header_buffer,
	    sizeof(header_buffer),
	    "HTTP/1.1 401 Unauthorized\r\n"
	    "WWW-Authenticate: Basic realm=\"%s\"\r\n"
	    "Content-Length: %zu\r\n"
	    "Connection: close\r\n\r\n"
	    "%s",
	    AUTH_REALM,
	    strlen(body),
	    body
	);

	send(client_socket, header_buffer, strlen(header_buffer), 0);
}

static void handle_list_files(const int client_socket)
{
	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
	{
		http_send_error(
		    client_socket, 500, "Internal Server Error", "Erro ao abrir diretório."
		);
		return;
	}

	char json_list[RESP_BUFFER_SIZE] = "[";
	size_t offset = 1;
	struct dirent *ent;

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		{
			continue;
		}

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

		struct stat st;
		if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode))
		{
			const char *const separator = (offset > 1) ? "," : "";
			offset += (size_t)snprintf(
			    json_list + offset,
			    sizeof(json_list) - offset,
			    "%s\"%s\"",
			    separator,
			    ent->d_name
			);
		}
	}
	closedir(dir);

	snprintf(json_list + offset, sizeof(json_list) - offset, "]");

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "application/json",
	    .body = json_list,
	    .body_len = strlen(json_list)
	};
	http_send_response(client_socket, &res);
}

static void handle_file_read(const int client_socket, const http_request_t *const req)
{
	const char *const filename = req->path + strlen(API_DATA_PREFIX);
	if (strchr(filename, '/') != nullptr)
	{
		http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	struct stat st;
	if (stat(filepath, &st) == -1)
	{
		http_send_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Arquivo inexistente."
		);
		return;
	}

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "text/plain",
	    .file_path = filepath
	};
	http_send_response(client_socket, &res);
}

static void handle_file_write(
    const int client_socket, const http_request_t *const req, const char *const mode,
    const char *const success_msg
)
{
	const char *const filename = req->path + strlen(API_DATA_PREFIX);
	if (strchr(filename, '/') != nullptr)
	{
		http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	FILE *const f = fopen(filepath, mode);
	if (!f)
	{
		http_send_error(client_socket, 500, "Internal Server Error", "Erro ao abrir arquivo.");
		return;
	}

	if (req->body)
	{
		fwrite(req->body, 1, strlen(req->body), f);
	}
	fclose(f);

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "text/plain",
	    .body = success_msg,
	    .body_len = strlen(success_msg)
	};
	http_send_response(client_socket, &res);
}

static void handle_file_delete(const int client_socket, const http_request_t *const req)
{
	const char *const filename = req->path + strlen(API_DATA_PREFIX);
	if (strchr(filename, '/') != nullptr)
	{
		http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	if (remove(filepath) == 0)
	{
		http_send_error(client_socket, HTTP_STATUS_OK, "OK", "Removido com sucesso.");
	}
	else
	{
		http_send_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Erro ao remover.");
	}
}

bool api_handle_request(const int client_socket, http_request_t *const req)
{
	static bool initialized = false;
	if (!initialized)
	{
		ensure_data_directory();
		initialized = true;
	}

	if (requires_auth(req) && !is_authenticated(req))
	{
		send_unauthorized(client_socket);
		return true;
	}

	if (strncmp(req->path, "/api/", 5) != 0)
	{
		return false;
	}

	if (strcmp(req->path, API_FILES_ROUTE) == 0 && req->method == HTTP_GET)
	{
		handle_list_files(client_socket);
		return true;
	}

	if (strncmp(req->path, API_DATA_PREFIX, strlen(API_DATA_PREFIX)) == 0)
	{
		switch (req->method)
		{
		case HTTP_GET:
			handle_file_read(client_socket, req);
			break;
		case HTTP_POST:
			handle_file_write(client_socket, req, "w", "Criado com sucesso.");
			break;
		case HTTP_PUT:
			handle_file_write(client_socket, req, "w", "Atualizado com sucesso.");
			break;
		case HTTP_PATCH:
			handle_file_write(client_socket, req, "a", "Modificado com sucesso.");
			break;
		case HTTP_DELETE:
			handle_file_delete(client_socket, req);
			break;
		default:
			http_send_error(client_socket, 405, "Method Not Allowed", "Método não permitido.");
			break;
		}
		return true;
	}

	http_send_error(client_socket, 404, "Not Found", "Rota inexistente.");
	return true;
}
