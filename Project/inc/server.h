#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>

/* ── Tipos de Rede ───────────────────────────────────────── */

typedef enum ip_mode
{
	IP_MODE_IPV4_ONLY,
	IP_MODE_IPV6_ONLY,
	IP_MODE_DUAL_STACK
} ip_mode_t;

/* ── Interface de Rede ───────────────────────────────────── */

[[nodiscard]] int server_init(const char *ip, const uint16_t port, const ip_mode_t mode);

#endif
