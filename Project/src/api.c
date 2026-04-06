#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
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

constexpr char ROOM_PREFIX[] = "sala_";
constexpr size_t ROOM_PREFIX_LEN = sizeof(ROOM_PREFIX) - 1;

constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t RESP_BUFFER_SIZE = 8192;
constexpr size_t AUTH_HEADER_BUFFER_SIZE = 1024;
constexpr size_t ROOM_READ_BUFFER_SIZE = 512;

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
		if (strcasecmp(req->headers[i].key, "Authorization") == 0)
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

static bool is_room_finished(const char *const filepath)
{
	bool finished = false;
	FILE *const f = fopen(filepath, "r");
	if (f)
	{
		char buffer[ROOM_READ_BUFFER_SIZE] = {};
		const size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, f);
		buffer[bytes_read] = '\0';
		fclose(f);

		if (strstr(buffer, "\"winner\":null") == nullptr)
		{
			finished = true;
		}
	}
	return finished;
}

static void purge_expired_rooms(void)
{
	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
		return;

	const time_t now = time(nullptr);
	struct dirent *ent;

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strncmp(ent->d_name, ROOM_PREFIX, ROOM_PREFIX_LEN) != 0)
			continue;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

		struct stat st;
		if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode))
		{
			const bool is_expired = (now - st.st_mtime > 240);
			const bool is_finished = is_room_finished(filepath);
			const bool grace_period_ended = (now - st.st_mtime > 10);

			if (is_expired || (is_finished && grace_period_ended))
			{
				remove(filepath);
			}
		}
	}
	closedir(dir);
}

static size_t get_active_room_count(void)
{
	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
		return 0;

	size_t count = 0;
	struct dirent *ent;

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strncmp(ent->d_name, ROOM_PREFIX, ROOM_PREFIX_LEN) == 0)
		{
			char filepath[PATH_BUFFER_SIZE];
			snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

			if (!is_room_finished(filepath))
			{
				count++;
			}
		}
	}
	closedir(dir);
	return count;
}

static void initialize_room_file(FILE *const f, const char *const player_name)
{
	const time_t now_seconds = time(nullptr);
	const long long now_ms = (long long)now_seconds * 1000;

	fprintf(
	    f,
	    "{\"board\":[\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"],"
	    "\"turn\":\"X\","
	    "\"players\":{\"X\":\"%.32s\",\"O\":null},"
	    "\"winner\":null,"
	    "\"createdAt\":%lld}",
	    player_name,
	    now_ms
	);
}

static void handle_list_files(const int client_socket)
{
	purge_expired_rooms();

	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
	{
		http_send_status(
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
			if (strncmp(ent->d_name, ROOM_PREFIX, ROOM_PREFIX_LEN) == 0)
			{
				if (is_room_finished(filepath))
				{
					continue;
				}
			}

			const char *const separator = (offset > 1) ? "," : "";
			const int written = snprintf(
			    json_list + offset,
			    sizeof(json_list) - offset,
			    "%s\"%s\"",
			    separator,
			    ent->d_name
			);
			if (written < 0 || (size_t)written >= (sizeof(json_list) - offset))
			{
				break;
			}
			offset += (size_t)written;
		}
	}
	closedir(dir);

	if ((sizeof(json_list) - offset) > 1)
	{
		snprintf(json_list + offset, sizeof(json_list) - offset, "]");
	}

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "application/json",
	    .mode = RES_MODE_MEMORY,
	    .body_len = strlen(json_list),
	    .body = json_list
	};
	http_send_response(client_socket, &res);
}

static void handle_file_read(const int client_socket, const http_request_t *const req)
{
	const char *const filename = req->path + strlen(API_DATA_PREFIX);
	if (strchr(filename, '/') != nullptr)
	{
		http_send_status(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	struct stat st;
	if (stat(filepath, &st) == -1)
	{
		http_send_status(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Arquivo inexistente."
		);
		return;
	}

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "application/json",
	    .mode = RES_MODE_FILE,
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
		http_send_status(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	const bool is_post = req->method == HTTP_POST;
	const bool is_room = strncmp(filename, ROOM_PREFIX, ROOM_PREFIX_LEN) == 0;
	const bool is_room_creation = is_post && is_room;

	if (is_room && req->body_len >= ROOM_READ_BUFFER_SIZE)
	{
		http_send_status(
		    client_socket, 413, "Payload Too Large", "Estado da sala excedeu o limite seguro."
		);
		return;
	}

	if (is_room_creation)
	{
		purge_expired_rooms();
		if (get_active_room_count() >= 5)
		{
			http_send_status(
			    client_socket, 429, "Too Many Requests", "Limite de 5 salas atingido."
			);
			return;
		}
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	FILE *const f = fopen(filepath, mode);
	if (!f)
	{
		http_send_status(client_socket, 500, "Internal Server Error", "Erro ao abrir arquivo.");
		return;
	}

	if (is_room_creation)
	{
		initialize_room_file(f, (req->body ? req->body : "Anônimo"));
	}
	else if (req->body && req->body_len > 0)
	{
		fwrite(req->body, 1, req->body_len, f);
	}

	fclose(f);

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "application/json",
	    .mode = RES_MODE_MEMORY,
	    .body_len = strlen(success_msg),
	    .body = success_msg
	};
	http_send_response(client_socket, &res);
}

static void handle_file_delete(const int client_socket, const http_request_t *const req)
{
	const char *const filename = req->path + strlen(API_DATA_PREFIX);
	if (strchr(filename, '/') != nullptr)
	{
		http_send_status(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	if (remove(filepath) == 0)
	{
		http_send_status(client_socket, HTTP_STATUS_OK, "OK", "Removido com sucesso.");
	}
	else
	{
		http_send_status(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Erro ao remover.");
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
			http_send_status(client_socket, 405, "Method Not Allowed", "Método não permitido.");
			break;
		}
		return true;
	}

	http_send_status(client_socket, 404, "Not Found", "Rota inexistente.");
	return true;
}
