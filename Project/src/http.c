#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "api.h"
#include "http.h"

constexpr char STATIC_DIR[] = "./web/";
constexpr size_t RECV_BUFFER_SIZE = 8192;
constexpr size_t PATH_BUFFER_SIZE = 1024;
constexpr size_t HEADER_BUFFER_SIZE = 1024;
constexpr size_t FILE_CHUNK_SIZE = 8192;

static void ensure_static_directory(void)
{
	struct stat st = {};
	if (stat(STATIC_DIR, &st) == -1)
	{
		mkdir(STATIC_DIR, 0755);
	}
}

static const char *get_mime_type(const char *const path)
{
	const char *const ext = strrchr(path, '.');
	if (!ext)
	{
		return "application/octet-stream";
	}

	static const struct
	{
		const char *const ext;
		const char *const type;
	} mime_map[] = {
	    {".html", "text/html"},
	    {".css", "text/css"},
	    {".js", "application/javascript"},
	    {".json", "application/json"},
	    {".png", "image/png"},
	    {".jpg", "image/jpeg"},
	    {".jpeg", "image/jpeg"},
	    {".ico", "image/x-icon"}
	};

	const size_t map_size = sizeof(mime_map) / sizeof(mime_map[0]);
	for (size_t i = 0; i < map_size; i++)
	{
		if (strcmp(ext, mime_map[i].ext) == 0)
		{
			return mime_map[i].type;
		}
	}

	return "application/octet-stream";
}

static http_method_t parse_method(const char *const method_str)
{
	static const struct
	{
		const char *const str;
		const http_method_t method;
	} method_map[] = {
	    {"GET", HTTP_GET},
	    {"POST", HTTP_POST},
	    {"PUT", HTTP_PUT},
	    {"PATCH", HTTP_PATCH},
	    {"DELETE", HTTP_DELETE}
	};

	const size_t map_size = sizeof(method_map) / sizeof(method_map[0]);
	for (size_t i = 0; i < map_size; i++)
	{
		if (strcmp(method_str, method_map[i].str) == 0)
		{
			return method_map[i].method;
		}
	}
	return HTTP_UNKNOWN;
}

static void parse_request_line(char *const line, http_request_t *const req)
{
	char *const method_str = line;
	char *const path_part = strchr(method_str, ' ');
	if (!path_part)
	{
		return;
	}

	*path_part = '\0';
	req->method = parse_method(method_str);

	char *path_start = path_part + 1;
	while (*path_start == ' ')
	{
		path_start++;
	}

	char *const version_part = strchr(path_start, ' ');
	if (version_part)
	{
		*version_part = '\0';
		char *version_start = version_part + 1;
		while (*version_start == ' ')
		{
			version_start++;
		}
		req->version = version_start;
	}

	char *const query = strchr(path_start, '?');
	if (query)
	{
		*query = '\0';
	}

	req->path = path_start;
}

static void parse_headers(http_request_t *const req, char **const saveptr)
{
	char *line;
	while ((line = strtok_r(nullptr, "\r\n", saveptr)) && req->header_count < MAX_HTTP_HEADERS)
	{
		char *const colon = strchr(line, ':');
		if (!colon)
		{
			continue;
		}

		*colon = '\0';
		req->headers[req->header_count].key = line;

		char *value_start = colon + 1;
		while (*value_start == ' ')
		{
			value_start++;
		}
		req->headers[req->header_count].value = value_start;
		req->header_count++;
	}
}

static void parse_http_request(char *const buffer, http_request_t *const req)
{
	req->header_count = 0;
	req->body = nullptr;
	req->body_len = 0;

	char *saveptr;
	char *const first_line = strtok_r(buffer, "\r\n", &saveptr);
	if (!first_line)
	{
		return;
	}

	parse_request_line(first_line, req);
	parse_headers(req, &saveptr);
}

static void send_file_content(const int client_socket, FILE *const file)
{
	char chunk[FILE_CHUNK_SIZE];
	size_t bytes_read;
	while ((bytes_read = fread(chunk, 1, sizeof(chunk), file)) > 0)
	{
		if (send(client_socket, chunk, bytes_read, 0) < 0)
		{
			break;
		}
	}
}

static void send_file_response(
    const int client_socket, const http_response_t *const res, char *const header_buffer,
    size_t offset
)
{
	FILE *const file = fopen(res->file_path, "rb");
	if (!file)
	{
		return;
	}

	fseek(file, 0, SEEK_END);
	const long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	const int cl_ret = snprintf(
	    header_buffer + offset,
	    HEADER_BUFFER_SIZE - offset,
	    "Content-Length: %ld\r\n\r\n",
	    file_size
	);

	if (cl_ret > 0)
	{
		offset += (size_t)cl_ret;
	}

	send(client_socket, header_buffer, offset, 0);
	send_file_content(client_socket, file);
	fclose(file);
}

static void send_body_response(
    const int client_socket, const http_response_t *const res, char *const header_buffer,
    size_t offset
)
{
	const int cl_ret = snprintf(
	    header_buffer + offset,
	    HEADER_BUFFER_SIZE - offset,
	    "Content-Length: %zu\r\n\r\n",
	    res->body_len
	);

	if (cl_ret > 0)
	{
		offset += (size_t)cl_ret;
	}

	send(client_socket, header_buffer, offset, 0);
	if (res->body && res->body_len > 0)
	{
		send(client_socket, res->body, res->body_len, 0);
	}
}

void http_send_response(const int client_socket, const http_response_t *const res)
{
	char header_buffer[HEADER_BUFFER_SIZE];
	size_t offset = 0;

	const int status_ret = snprintf(
	    header_buffer + offset,
	    sizeof(header_buffer) - offset,
	    "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nConnection: keep-alive\r\n",
	    res->status_code,
	    res->status_message,
	    res->content_type
	);

	if (status_ret > 0)
	{
		offset += (size_t)status_ret;
	}

	for (size_t i = 0; i < res->header_count && i < MAX_HTTP_HEADERS; i++)
	{
		const int h_ret = snprintf(
		    header_buffer + offset,
		    sizeof(header_buffer) - offset,
		    "%s: %s\r\n",
		    res->headers[i].key,
		    res->headers[i].value
		);
		if (h_ret > 0)
		{
			offset += (size_t)h_ret;
		}
	}

	if (res->file_path)
	{
		send_file_response(client_socket, res, header_buffer, offset);
	}
	else
	{
		send_body_response(client_socket, res, header_buffer, offset);
	}
}

void http_send_error(
    const int client_socket, const int status_code, const char *const status_msg,
    const char *const body
)
{
	const http_response_t res = {
	    .status_code = status_code,
	    .status_message = status_msg,
	    .content_type = "text/plain",
	    .body = body,
	    .body_len = strlen(body)
	};
	http_send_response(client_socket, &res);
}

static void serve_static_file(const int client_socket, const http_request_t *const req)
{
	char target_file[PATH_BUFFER_SIZE];
	const char *const relative_path = (strcmp(req->path, "/") == 0) ? "index.html"
	                                                                : req->path + 1;

	snprintf(target_file, sizeof(target_file), "%s%s", STATIC_DIR, relative_path);

	struct stat st_file;
	if (stat(target_file, &st_file) == -1)
	{
		char temp_file[PATH_BUFFER_SIZE];
		snprintf(temp_file, sizeof(temp_file), "%s%s.html", STATIC_DIR, relative_path);

		if (stat(temp_file, &st_file) == 0)
		{
			strcpy(target_file, temp_file);
		}
	}

	if (stat(target_file, &st_file) == -1)
	{
		http_send_error(
		    client_socket, HTTP_STATUS_NOT_FOUND, "Not Found", "Página não encontrada."
		);
		return;
	}

	const http_response_t res = {
	    .status_code = HTTP_STATUS_OK,
	    .status_message = "OK",
	    .content_type = get_mime_type(target_file),
	    .file_path = target_file
	};
	http_send_response(client_socket, &res);
}

static void configure_socket_timeout(const int client_socket, const time_t seconds)
{
	const struct timeval tv = {.tv_sec = seconds, .tv_usec = 0};
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static bool read_http_headers(
    const int client_socket, char *const buffer, const size_t buffer_size,
    size_t *const out_total_received, char **const out_separator
)
{
	size_t total_received = 0;
	while (total_received < buffer_size - 1)
	{
		const ssize_t bytes = recv(
		    client_socket, buffer + total_received, buffer_size - 1 - total_received, 0
		);
		if (bytes <= 0)
		{
			return false;
		}

		total_received += (size_t)bytes;
		buffer[total_received] = '\0';

		const char *const sep = strstr(buffer, "\r\n\r\n");
		if (sep)
		{
			*out_separator = (char *)sep;
			*out_total_received = total_received;
			return true;
		}
	}
	return false;
}

static size_t get_content_length(const http_request_t *const req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].key, "Content-Length") == 0)
		{
			return (size_t)strtoul(req->headers[i].value, nullptr, 10);
		}
	}
	return 0;
}

static bool read_http_body(
    const int client_socket, http_request_t *const req, const char *const partial_body_start,
    const size_t partial_len
)
{
	if (req->body_len == 0)
	{
		return true;
	}

	req->body = (char *)malloc(req->body_len + 1);
	if (!req->body)
	{
		return false;
	}

	size_t current_len = (partial_len > req->body_len) ? req->body_len : partial_len;
	if (current_len > 0)
	{
		memcpy(req->body, partial_body_start, current_len);
	}

	while (current_len < req->body_len)
	{
		const ssize_t bytes = recv(
		    client_socket, req->body + current_len, req->body_len - current_len, 0
		);
		if (bytes <= 0)
		{
			return false;
		}
		current_len += (size_t)bytes;
	}

	req->body[current_len] = '\0';
	return true;
}

static void route_request(const int client_socket, http_request_t *const req)
{
	if (!req->path)
	{
		return;
	}

	if (strstr(req->path, ".."))
	{
		http_send_error(client_socket, HTTP_STATUS_FORBIDDEN, "Forbidden", "Acesso Negado.");
		return;
	}

	if (api_handle_request(client_socket, req))
	{
		return;
	}

	if (req->method == HTTP_GET)
	{
		serve_static_file(client_socket, req);
		return;
	}

	http_send_error(
	    client_socket, HTTP_STATUS_NOT_ALLOWED, "Method Not Allowed", "Método não suportado."
	);
}

static bool should_keep_alive(const http_request_t *const req)
{
	for (size_t i = 0; i < req->header_count; i++)
	{
		if (strcasecmp(req->headers[i].key, "Connection") == 0)
		{
			return strcasecmp(req->headers[i].value, "close") != 0;
		}
	}
	return true;
}

void http_handle_client(const int client_socket)
{
	static bool initialized = false;
	if (!initialized)
	{
		ensure_static_directory();
		initialized = true;
	}

	configure_socket_timeout(client_socket, 4);

	bool keep_alive = true;
	while (keep_alive)
	{
		char buffer[RECV_BUFFER_SIZE] = {};
		size_t received = 0;
		char *separator = nullptr;

		if (!read_http_headers(client_socket, buffer, sizeof(buffer), &received, &separator))
		{
			break;
		}

		*separator = '\0';
		const char *const body_start = separator + 4;
		const size_t header_len = (size_t)(body_start - buffer);

		http_request_t req = {};
		parse_http_request(buffer, &req);
		req.body_len = get_content_length(&req);

		if (!read_http_body(client_socket, &req, body_start, received - header_len))
		{
			if (req.body)
			{
				free(req.body);
			}
			break;
		}

		route_request(client_socket, &req);
		keep_alive = should_keep_alive(&req);

		if (req.body)
		{
			free(req.body);
		}
	}
}
