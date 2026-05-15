#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <stddef.h>

/* ── Constantes ──────────────────────────────────────────── */

constexpr size_t AUTH_TOKEN_SIZE = 81;
constexpr size_t AUTH_MAX_CREDENTIAL = 64;

/* ── Funções de Autenticação ─────────────────────────────── */

bool auth_init(void);

[[nodiscard]] bool auth_login(
    const char *username, const char *password, char *out_token, size_t token_size
);

[[nodiscard]] bool auth_validate_token(const char *token);

#endif
