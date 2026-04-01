#ifndef API_H
#define API_H

#include "http.h"
#include <stddef.h>
#include <stdint.h>

[[nodiscard]] bool api_handle_request(const int client_socket, http_request_t *const req);

#endif
