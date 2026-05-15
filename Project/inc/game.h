#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* ── Constantes do Jogo ──────────────────────────────────── */

constexpr size_t BOARD_SIZE = 9;
constexpr size_t MAX_PLAYER_NAME = 33;
constexpr size_t MAX_ROOMS = 16;
constexpr time_t ROOM_DURATION_SECS = 300;
constexpr time_t ROOM_GRACE_PERIOD_SECS = 10;
constexpr size_t ROOM_ID_SIZE = 7;
constexpr size_t GAME_JSON_BUFFER = 1024;
constexpr size_t GAME_LIST_BUFFER = 8192;

/* ── Tipos de Dados ──────────────────────────────────────── */

typedef enum game_symbol
{
	SYMBOL_NONE = 0,
	SYMBOL_X,
	SYMBOL_O
} game_symbol_t;

typedef struct game_room
{
	char board[BOARD_SIZE];
	game_symbol_t turn;
	char player_x[MAX_PLAYER_NAME];
	char player_o[MAX_PLAYER_NAME];
	game_symbol_t winner;
	bool is_draw;
	time_t created_at;
} game_room_t;

typedef enum game_error
{
	GAME_OK = 0,
	GAME_ERR_NOT_YOUR_TURN,
	GAME_ERR_CELL_OCCUPIED,
	GAME_ERR_GAME_OVER,
	GAME_ERR_INVALID_CELL,
	GAME_ERR_INVALID_SYMBOL,
	GAME_ERR_ROOM_FULL,
	GAME_ERR_ROOM_NOT_FOUND,
	GAME_ERR_MAX_ROOMS,
	GAME_ERR_PARSE,
	GAME_ERR_IO
} game_error_t;

/* ── Interface do Motor de Jogo ──────────────────────────── */

[[nodiscard]] game_error_t game_create_room(
    const char *room_id, const char *player_name, char *out_json, size_t json_size
);

[[nodiscard]] game_error_t game_join_room(
    const char *room_id, const char *player_name, game_symbol_t *out_symbol
);

[[nodiscard]] game_error_t game_make_move(
    const char *room_id, const char *player_name, game_symbol_t symbol, size_t cell_index
);

[[nodiscard]] game_error_t game_get_state(
    const char *room_id, char *out_json, size_t json_size
);

[[nodiscard]] game_error_t game_list_rooms(char *out_json, size_t json_size);

void game_purge_expired(void);

[[nodiscard]] size_t game_active_count(void);

[[nodiscard]] const char *game_error_string(game_error_t err);

void game_ensure_data_dir(void);

#endif
