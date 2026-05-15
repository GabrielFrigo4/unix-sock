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
#include "auth.h"
#include "game.h"
#include "http.h"

/* ── Constantes ──────────────────────────────────────────── */

constexpr char DATA_DIR[] = "./data/";

constexpr char ROUTE_ROOMS[] = "/api/rooms";
constexpr char ROUTE_ROOMS_SLASH[] = "/api/rooms/";
constexpr size_t ROUTE_ROOMS_SLASH_LEN = sizeof(ROUTE_ROOMS_SLASH) - 1;
constexpr char ROUTE_AUTH_LOGIN[] = "/api/auth/login";
constexpr char ROUTE_DATA_PREFIX[] = "/api/data/";
constexpr size_t ROUTE_DATA_PREFIX_LEN = sizeof(ROUTE_DATA_PREFIX) - 1;
constexpr char ROUTE_FILES[] = "/api/files";

constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t RESP_BUFFER_SIZE = 8192;
constexpr size_t JSON_FIELD_SIZE = 128;

/* ── Helpers de JSON Request Body ────────────────────────── */

static bool body_extract_string(const char *body, const char *key, char *out, size_t out_size)
{
	if (!body)
		return false;

	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\":", key);

	const char *start = strstr(body, pattern);
	if (!start)
		return false;

	start += strlen(pattern);
	while (*start == ' ')
		start++;

	if (*start != '"')
		return false;
	start++;

	const char *end = strchr(start, '"');
	if (!end)
		return false;

	size_t len = (size_t)(end - start);
	if (len >= out_size)
		len = out_size - 1;

	memcpy(out, start, len);
	out[len] = '\0';
	return true;
}

static bool body_extract_int(const char *body, const char *key, long long *out)
{
	if (!body)
		return false;

	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\":", key);

	const char *start = strstr(body, pattern);
	if (!start)
		return false;

	start += strlen(pattern);
	while (*start == ' ')
		start++;

	*out = strtoll(start, nullptr, 10);
	return true;
}

/* ── Helpers de autenticação ─────────────────────────────── */

static bool extract_bearer_token(const http_request_t *const req, const char **out_token)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].key, "Authorization") == 0)
		{
			if (strncmp(req->headers[i].value, "Bearer ", 7) == 0)
			{
				*out_token = req->headers[i].value + 7;
				return true;
			}
		}
	}
	return false;
}

static bool is_admin_authenticated(const http_request_t *const req)
{
	const char *token = nullptr;
	if (!extract_bearer_token(req, &token))
		return false;
	return auth_validate_token(token);
}

static void send_json_error(
    const int client_socket, const int status_code, const char *status_msg,
    const char *error_msg
)
{
	char body[512];
	snprintf(body, sizeof(body), "{\"error\":\"%s\"}", error_msg);

	const http_response_t res = {
	    .status_code = status_code,
	    .status_message = status_msg,
	    .content_type = "application/json",
	    .mode = RES_MODE_MEMORY,
	    .body_len = strlen(body),
	    .body = body
	};
	http_send_response(client_socket, &res);
}

static void send_json_body(const int client_socket, const char *json)
{
	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = "application/json",
	    .mode = RES_MODE_MEMORY,
	    .body_len = strlen(json),
	    .body = json
	};
	http_send_response(client_socket, &res);
}

/* ── Handlers: Jogo ──────────────────────────────────────── */

static void handle_create_room(const int client_socket, const http_request_t *const req)
{
	char player_name[MAX_PLAYER_NAME] = {};
	if (!body_extract_string(req->body, "player", player_name, sizeof(player_name)) ||
	    player_name[0] == '\0')
	{
		send_json_error(
		    client_socket, HTTP_STATUS_BAD_REQUEST, "Bad Request", "Campo 'player' obrigatório."
		);
		return;
	}

	/* Gerar room ID */
	const unsigned int seed = (unsigned int)time(nullptr) ^ (unsigned int)getpid();
	srand(seed);

	char room_id[ROOM_ID_SIZE];
	constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	constexpr size_t charset_len = sizeof(charset) - 1;

	for (size_t i = 0; i < ROOM_ID_SIZE - 1; i++)
	{
		room_id[i] = charset[(size_t)rand() % charset_len];
	}
	room_id[ROOM_ID_SIZE - 1] = '\0';

	char room_json[GAME_JSON_BUFFER] = {};
	const game_error_t err = game_create_room(
	    room_id, player_name, room_json, sizeof(room_json)
	);

	if (err == GAME_ERR_MAX_ROOMS)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_TOO_MANY, "Too Many Requests", game_error_string(err)
		);
		return;
	}

	if (err != GAME_OK)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_CONFLICT, "Conflict", game_error_string(err)
		);
		return;
	}

	/* Responder com room_id + symbol + estado */
	char response[RESP_BUFFER_SIZE];
	snprintf(
	    response,
	    sizeof(response),
	    "{\"room_id\":\"%.6s\",\"symbol\":\"X\",\"state\":%s}",
	    room_id,
	    room_json
	);

	send_json_body(client_socket, response);
}

static void handle_join_room(
    const int client_socket, const http_request_t *const req, const char *room_id
)
{
	char player_name[MAX_PLAYER_NAME] = {};
	if (!body_extract_string(req->body, "player", player_name, sizeof(player_name)) ||
	    player_name[0] == '\0')
	{
		send_json_error(
		    client_socket, HTTP_STATUS_BAD_REQUEST, "Bad Request", "Campo 'player' obrigatório."
		);
		return;
	}

	game_symbol_t symbol = SYMBOL_NONE;
	const game_error_t err = game_join_room(room_id, player_name, &symbol);

	if (err == GAME_ERR_ROOM_NOT_FOUND)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", game_error_string(err)
		);
		return;
	}

	if (err == GAME_ERR_ROOM_FULL)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_CONFLICT, "Conflict", game_error_string(err)
		);
		return;
	}

	if (err != GAME_OK)
	{
		send_json_error(client_socket, 500, "Internal Server Error", game_error_string(err));
		return;
	}

	const char *symbol_str = (symbol == SYMBOL_X) ? "X" : "O";
	char response[256];
	snprintf(response, sizeof(response), "{\"symbol\":\"%s\"}", symbol_str);

	send_json_body(client_socket, response);
}

static void handle_make_move(
    const int client_socket, const http_request_t *const req, const char *room_id
)
{
	char player_name[MAX_PLAYER_NAME] = {};
	char symbol_str[4] = {};
	long long cell = -1;

	if (!body_extract_string(req->body, "player", player_name, sizeof(player_name)) ||
	    !body_extract_string(req->body, "symbol", symbol_str, sizeof(symbol_str)) ||
	    !body_extract_int(req->body, "cell", &cell))
	{
		send_json_error(
		    client_socket,
		    HTTP_STATUS_BAD_REQUEST,
		    "Bad Request",
		    "Campos 'player', 'symbol' e 'cell' obrigatórios."
		);
		return;
	}

	game_symbol_t symbol = SYMBOL_NONE;
	if (symbol_str[0] == 'X')
		symbol = SYMBOL_X;
	else if (symbol_str[0] == 'O')
		symbol = SYMBOL_O;
	else
	{
		send_json_error(
		    client_socket,
		    HTTP_STATUS_BAD_REQUEST,
		    "Bad Request",
		    "Símbolo deve ser 'X' ou 'O'."
		);
		return;
	}

	if (cell < 0 || cell > 8)
	{
		send_json_error(
		    client_socket,
		    HTTP_STATUS_BAD_REQUEST,
		    "Bad Request",
		    "Célula deve ser entre 0 e 8."
		);
		return;
	}

	const game_error_t err = game_make_move(room_id, player_name, symbol, (size_t)cell);

	if (err == GAME_ERR_ROOM_NOT_FOUND)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", game_error_string(err)
		);
		return;
	}

	if (err != GAME_OK)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_CONFLICT, "Conflict", game_error_string(err)
		);
		return;
	}

	/* Retornar estado atualizado */
	char state_json[GAME_JSON_BUFFER] = {};
	const game_error_t state_err = game_get_state(room_id, state_json, sizeof(state_json));
	(void)state_err;
	send_json_body(client_socket, state_json);
}

static void handle_get_room(const int client_socket, const char *room_id)
{
	char json[GAME_JSON_BUFFER] = {};
	const game_error_t err = game_get_state(room_id, json, sizeof(json));

	if (err == GAME_ERR_ROOM_NOT_FOUND)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", game_error_string(err)
		);
		return;
	}

	send_json_body(client_socket, json);
}

static void handle_list_rooms(const int client_socket)
{
	char json[GAME_LIST_BUFFER] = {};
	const game_error_t err = game_list_rooms(json, sizeof(json));
	(void)err;
	send_json_body(client_socket, json);
}

/* ── Handlers: Auth ──────────────────────────────────────── */

static void handle_auth_login(const int client_socket, const http_request_t *const req)
{
	char username[JSON_FIELD_SIZE] = {};
	char password[JSON_FIELD_SIZE] = {};

	if (!body_extract_string(req->body, "user", username, sizeof(username)) ||
	    !body_extract_string(req->body, "pass", password, sizeof(password)))
	{
		send_json_error(
		    client_socket,
		    HTTP_STATUS_BAD_REQUEST,
		    "Bad Request",
		    "Campos 'user' e 'pass' obrigatórios."
		);
		return;
	}

	char token[AUTH_TOKEN_SIZE] = {};
	if (!auth_login(username, password, token, sizeof(token)))
	{
		send_json_error(
		    client_socket, HTTP_STATUS_UNAUTHORIZED, "Unauthorized", "Credenciais inválidas."
		);
		return;
	}

	char response[256];
	snprintf(response, sizeof(response), "{\"token\":\"%s\"}", token);
	send_json_body(client_socket, response);
}

/* ── Handlers: Admin File Ops (com auth) ─────────────────── */

static void ensure_data_directory(void)
{
	struct stat st = {};
	if (stat(DATA_DIR, &st) == -1)
	{
		mkdir(DATA_DIR, 0755);
	}
}

static void handle_admin_list_files(const int client_socket)
{
	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
	{
		send_json_error(
		    client_socket, 500, "Internal Server Error", "Erro ao abrir diretório."
		);
		return;
	}

	char json_list[RESP_BUFFER_SIZE] = {};
	size_t offset = 0;
	offset += (size_t)snprintf(json_list, sizeof(json_list), "[");

	struct dirent *ent;
	while ((ent = readdir(dir)) != nullptr)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;

		char filepath[PATH_BUFFER_SIZE];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

		struct stat st;
		if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode))
			continue;

		const char *separator = (offset > 1) ? "," : "";
		const int written = snprintf(
		    json_list + offset, sizeof(json_list) - offset, "%s\"%s\"", separator, ent->d_name
		);
		if (written < 0 || (size_t)written >= (sizeof(json_list) - offset))
			break;
		offset += (size_t)written;
	}
	closedir(dir);

	snprintf(json_list + offset, sizeof(json_list) - offset, "]");
	send_json_body(client_socket, json_list);
}

static void handle_admin_file_read(const int client_socket, const char *filename)
{
	if (strchr(filename, '/') != nullptr)
	{
		send_json_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	struct stat st;
	if (stat(filepath, &st) == -1)
	{
		send_json_error(
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

static void handle_admin_file_write(
    const int client_socket, const http_request_t *const req, const char *filename,
    const char *mode
)
{
	if (strchr(filename, '/') != nullptr)
	{
		send_json_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	FILE *const f = fopen(filepath, mode);
	if (!f)
	{
		send_json_error(client_socket, 500, "Internal Server Error", "Erro ao abrir arquivo.");
		return;
	}

	if (req->body && req->body_len > 0)
	{
		fwrite(req->body, 1, req->body_len, f);
	}
	fclose(f);

	send_json_body(client_socket, "{\"ok\":true}");
}

static void handle_admin_file_delete(const int client_socket, const char *filename)
{
	if (strchr(filename, '/') != nullptr)
	{
		send_json_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso negado.");
		return;
	}

	char filepath[PATH_BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, filename);

	if (remove(filepath) == 0)
	{
		send_json_body(client_socket, "{\"ok\":true}");
	}
	else
	{
		send_json_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Erro ao remover.");
	}
}

/* ── Router de rotas /api/rooms/{id}/... ─────────────────── */

static bool route_room_sub(
    const int client_socket, const http_request_t *const req, const char *path_after_rooms
)
{
	/* Extrair room_id: tudo até próximo '/' ou fim */
	const char *slash = strchr(path_after_rooms, '/');

	char room_id[ROOM_ID_SIZE] = {};
	size_t id_len;

	if (slash)
	{
		id_len = (size_t)(slash - path_after_rooms);
	}
	else
	{
		id_len = strlen(path_after_rooms);
	}

	if (id_len == 0 || id_len >= ROOM_ID_SIZE)
	{
		send_json_error(
		    client_socket, HTTP_STATUS_BAD_REQUEST, "Bad Request", "Room ID inválido."
		);
		return true;
	}

	memcpy(room_id, path_after_rooms, id_len);
	room_id[id_len] = '\0';

	if (!slash)
	{
		/* GET /api/rooms/{id} */
		if (req->method == HTTP_GET)
		{
			handle_get_room(client_socket, room_id);
			return true;
		}
		send_json_error(
		    client_socket,
		    HTTP_STATUS_NOT_ALLOWED,
		    "Method Not Allowed",
		    "Método não permitido."
		);
		return true;
	}

	const char *action = slash + 1;

	if (strcmp(action, "join") == 0 && req->method == HTTP_POST)
	{
		handle_join_room(client_socket, req, room_id);
		return true;
	}

	if (strcmp(action, "move") == 0 && req->method == HTTP_POST)
	{
		handle_make_move(client_socket, req, room_id);
		return true;
	}

	send_json_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota inexistente.");
	return true;
}

/* ── Entry Point ─────────────────────────────────────────── */

void api_init(void)
{
	ensure_data_directory();
	game_ensure_data_dir();
	auth_init();
}

bool api_handle_request(const int client_socket, http_request_t *const req)
{
	if (strncmp(req->path, "/api/", 5) != 0)
	{
		return false;
	}

	/* ── Rotas públicas do jogo ─────────────────────────── */

	/* POST /api/rooms → criar sala */
	if (strcmp(req->path, ROUTE_ROOMS) == 0 && req->method == HTTP_POST)
	{
		handle_create_room(client_socket, req);
		return true;
	}

	/* GET /api/rooms → listar salas */
	if (strcmp(req->path, ROUTE_ROOMS) == 0 && req->method == HTTP_GET)
	{
		handle_list_rooms(client_socket);
		return true;
	}

	/* /api/rooms/{id}... */
	if (strncmp(req->path, ROUTE_ROOMS_SLASH, ROUTE_ROOMS_SLASH_LEN) == 0)
	{
		return route_room_sub(client_socket, req, req->path + ROUTE_ROOMS_SLASH_LEN);
	}

	/* ── Rota de login ──────────────────────────────────── */

	if (strcmp(req->path, ROUTE_AUTH_LOGIN) == 0 && req->method == HTTP_POST)
	{
		handle_auth_login(client_socket, req);
		return true;
	}

	/* ── Rotas admin (requerem Bearer Token) ────────────── */

	const bool is_admin_route = strcmp(req->path, ROUTE_FILES) == 0 ||
	                            strncmp(req->path, ROUTE_DATA_PREFIX, ROUTE_DATA_PREFIX_LEN) ==
	                                0;

	if (is_admin_route && !is_admin_authenticated(req))
	{
		send_json_error(
		    client_socket,
		    HTTP_STATUS_UNAUTHORIZED,
		    "Unauthorized",
		    "Token de acesso inválido ou ausente."
		);
		return true;
	}

	if (strcmp(req->path, ROUTE_FILES) == 0 && req->method == HTTP_GET)
	{
		handle_admin_list_files(client_socket);
		return true;
	}

	if (strncmp(req->path, ROUTE_DATA_PREFIX, ROUTE_DATA_PREFIX_LEN) == 0)
	{
		const char *filename = req->path + ROUTE_DATA_PREFIX_LEN;

		switch (req->method)
		{
		case HTTP_GET:
			handle_admin_file_read(client_socket, filename);
			break;
		case HTTP_POST:
			handle_admin_file_write(client_socket, req, filename, "w");
			break;
		case HTTP_PUT:
			handle_admin_file_write(client_socket, req, filename, "w");
			break;
		case HTTP_DELETE:
			handle_admin_file_delete(client_socket, filename);
			break;
		default:
			send_json_error(
			    client_socket,
			    HTTP_STATUS_NOT_ALLOWED,
			    "Method Not Allowed",
			    "Método não permitido."
			);
			break;
		}
		return true;
	}

	send_json_error(client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Rota inexistente.");
	return true;
}
