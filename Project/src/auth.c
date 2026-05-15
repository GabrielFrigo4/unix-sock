#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/evp.h>

#include "auth.h"

/* ── Definições e Estado ─────────────────────────────────── */

constexpr size_t SECRET_SIZE = 32;
constexpr size_t HASH_SIZE = 32;
constexpr size_t TIMESTAMP_HEX_LEN = 16;
constexpr size_t HMAC_HEX_LEN = 64;
constexpr time_t TOKEN_TTL_SECS = 3600;

static unsigned char server_secret[SECRET_SIZE];
static char admin_user[AUTH_MAX_CREDENTIAL];
static unsigned char admin_pass_hash[HASH_SIZE];
static bool initialized = false;

/* ── Helpers internos ────────────────────────────────────── */

static void bytes_to_hex(const unsigned char *in, size_t in_len, char *out)
{
	for (size_t i = 0; i < in_len; i++)
	{
		snprintf(out + (i * 2), 3, "%02x", in[i]);
	}
}

static bool hex_to_bytes(const char *hex, unsigned char *out, size_t out_len)
{
	for (size_t i = 0; i < out_len; i++)
	{
		unsigned int byte_val = 0;
		if (sscanf(hex + (i * 2), "%2x", &byte_val) != 1)
			return false;
		out[i] = (unsigned char)byte_val;
	}
	return true;
}

static bool compute_sha256(const void *data, size_t data_len, unsigned char *out)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	if (!ctx)
		return false;

	unsigned int out_len = 0;
	const int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) &&
	               EVP_DigestUpdate(ctx, data, data_len) &&
	               EVP_DigestFinal_ex(ctx, out, &out_len);

	EVP_MD_CTX_free(ctx);
	return ok == 1;
}

static bool compute_hmac_sha256(
    const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len,
    unsigned char *out
)
{
	EVP_MAC *mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
	if (!mac)
		return false;

	EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
	EVP_MAC_free(mac);
	if (!ctx)
		return false;

	char digest_name[] = "SHA256";
	OSSL_PARAM params[] = {
	    OSSL_PARAM_construct_utf8_string("digest", digest_name, 0), OSSL_PARAM_construct_end()
	};

	size_t out_len = HASH_SIZE;
	const int ok = EVP_MAC_init(ctx, key, key_len, params) &&
	               EVP_MAC_update(ctx, data, data_len) &&
	               EVP_MAC_final(ctx, out, &out_len, HASH_SIZE);

	EVP_MAC_CTX_free(ctx);
	return ok == 1;
}

static bool timing_safe_compare(const unsigned char *a, const unsigned char *b, size_t len)
{
	volatile unsigned char diff = 0;
	for (size_t i = 0; i < len; i++)
	{
		diff |= a[i] ^ b[i];
	}
	return diff == 0;
}

/* ── Geração de token ────────────────────────────────────── */

static bool generate_token(const char *username, char *out_token, size_t token_size)
{
	if (token_size < AUTH_TOKEN_SIZE)
		return false;

	const uint64_t now = (uint64_t)time(nullptr);
	char timestamp_hex[TIMESTAMP_HEX_LEN + 1];
	snprintf(timestamp_hex, sizeof(timestamp_hex), "%016llx", (unsigned long long)now);

	/* payload = username + ":" + timestamp_hex */
	char payload[256];
	snprintf(payload, sizeof(payload), "%s:%s", username, timestamp_hex);

	unsigned char hmac_out[HASH_SIZE];
	if (!compute_hmac_sha256(
	        server_secret,
	        SECRET_SIZE,
	        (const unsigned char *)payload,
	        strlen(payload),
	        hmac_out
	    ))
	{
		return false;
	}

	char hmac_hex[HMAC_HEX_LEN + 1];
	bytes_to_hex(hmac_out, HASH_SIZE, hmac_hex);
	hmac_hex[HMAC_HEX_LEN] = '\0';

	snprintf(out_token, token_size, "%s%s", timestamp_hex, hmac_hex);
	return true;
}

/* ── Interface Pública ───────────────────────────────────── */

bool auth_init(void)
{
	const char *user = getenv("ADMIN_USER");
	const char *pass = getenv("ADMIN_PASS");

	if (!user || user[0] == '\0')
	{
		user = "admin";
		fprintf(stderr, "[WARN]: ADMIN_USER não definido, usando 'admin'.\n");
	}

	if (!pass || pass[0] == '\0')
	{
		pass = "admin";
		fprintf(stderr, "[WARN]: ADMIN_PASS não definido, usando 'admin'.\n");
	}

	strncpy(admin_user, user, sizeof(admin_user) - 1);
	admin_user[sizeof(admin_user) - 1] = '\0';

	if (!compute_sha256(pass, strlen(pass), admin_pass_hash))
	{
		fprintf(stderr, "[ERRO]: Falha ao gerar hash da senha admin.\n");
		return false;
	}

	/* Gerar secret aleatório */
	FILE *const urandom = fopen("/dev/urandom", "rb");
	if (!urandom)
	{
		perror("[ERRO]: fopen /dev/urandom");
		return false;
	}

	const size_t read_bytes = fread(server_secret, 1, SECRET_SIZE, urandom);
	fclose(urandom);

	if (read_bytes != SECRET_SIZE)
	{
		fprintf(stderr, "[ERRO]: Falha ao ler /dev/urandom.\n");
		return false;
	}

	initialized = true;
	printf("[INFO]: Módulo de autenticação inicializado (user: %s).\n", admin_user);
	return true;
}

bool auth_login(const char *username, const char *password, char *out_token, size_t token_size)
{
	if (!initialized)
		return false;

	/* Verificar username */
	if (strcmp(username, admin_user) != 0)
		return false;

	/* Verificar senha via hash */
	unsigned char input_hash[HASH_SIZE];
	if (!compute_sha256(password, strlen(password), input_hash))
		return false;

	if (!timing_safe_compare(input_hash, admin_pass_hash, HASH_SIZE))
		return false;

	return generate_token(username, out_token, token_size);
}

bool auth_validate_token(const char *token)
{
	if (!initialized)
		return false;

	const size_t token_len = strlen(token);
	if (token_len != TIMESTAMP_HEX_LEN + HMAC_HEX_LEN)
		return false;

	/* Extrair timestamp */
	char timestamp_hex[TIMESTAMP_HEX_LEN + 1] = {};
	memcpy(timestamp_hex, token, TIMESTAMP_HEX_LEN);
	timestamp_hex[TIMESTAMP_HEX_LEN] = '\0';

	unsigned long long ts_val = 0;
	if (sscanf(timestamp_hex, "%llx", &ts_val) != 1)
		return false;

	/* Verificar expiração */
	const time_t now = time(nullptr);
	if (now - (time_t)ts_val > TOKEN_TTL_SECS)
		return false;
	if ((time_t)ts_val > now + 60)
		return false; /* Token do futuro? */

	/* Recomputar HMAC */
	char payload[256];
	snprintf(payload, sizeof(payload), "%s:%s", admin_user, timestamp_hex);

	unsigned char expected_hmac[HASH_SIZE];
	if (!compute_hmac_sha256(
	        server_secret,
	        SECRET_SIZE,
	        (const unsigned char *)payload,
	        strlen(payload),
	        expected_hmac
	    ))
	{
		return false;
	}

	/* Extrair HMAC recebido */
	unsigned char received_hmac[HASH_SIZE];
	if (!hex_to_bytes(token + TIMESTAMP_HEX_LEN, received_hmac, HASH_SIZE))
		return false;

	return timing_safe_compare(expected_hmac, received_hmac, HASH_SIZE);
}
