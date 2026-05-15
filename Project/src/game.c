#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "game.h"

/* ── Definições e Estado ─────────────────────────────────── */

constexpr char DATA_DIR[] = "./data/";
constexpr char ROOM_PREFIX[] = "sala_";
constexpr size_t ROOM_PREFIX_LEN = sizeof(ROOM_PREFIX) - 1;
constexpr char ROOM_SUFFIX[] = ".json";
constexpr size_t PATH_BUF = 256;
constexpr size_t FILE_BUF = 1024;
/* ── Combinações vencedoras ──────────────────────────────── */

static const size_t WIN_COMBOS[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}
};

/* ── Helpers internos ────────────────────────────────────── */

static void build_filepath(char *out, size_t out_size, const char *room_id)
{
	snprintf(out, out_size, "%ssala_%s.json", DATA_DIR, room_id);
}

static char symbol_to_char(const game_symbol_t s)
{
	if (s == SYMBOL_X)
		return 'X';
	if (s == SYMBOL_O)
		return 'O';
	return '\0';
}

static game_symbol_t char_to_symbol(const char c)
{
	if (c == 'X')
		return SYMBOL_X;
	if (c == 'O')
		return SYMBOL_O;
	return SYMBOL_NONE;
}

/* ── JSON Parsing (formato controlado) ───────────────────── */

static bool json_extract_string(const char *json, const char *key, char *out, size_t out_size)
{
	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

	const char *start = strstr(json, pattern);
	if (!start)
		return false;

	start += strlen(pattern);
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

static bool json_extract_nullable_string(
    const char *json, const char *key, char *out, size_t out_size, bool *is_null
)
{
	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\":", key);

	const char *start = strstr(json, pattern);
	if (!start)
		return false;

	start += strlen(pattern);

	if (strncmp(start, "null", 4) == 0)
	{
		*is_null = true;
		out[0] = '\0';
		return true;
	}

	*is_null = false;
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

static bool json_extract_long(const char *json, const char *key, long long *out)
{
	char pattern[128];
	snprintf(pattern, sizeof(pattern), "\"%s\":", key);

	const char *start = strstr(json, pattern);
	if (!start)
		return false;

	start += strlen(pattern);
	*out = strtoll(start, nullptr, 10);
	return true;
}

static bool parse_board(const char *json, char board[BOARD_SIZE])
{
	const char *pos = strstr(json, "\"board\":[");
	if (!pos)
		return false;

	pos += strlen("\"board\":[");

	for (size_t i = 0; i < BOARD_SIZE; i++)
	{
		while (*pos == ' ' || *pos == ',')
			pos++;

		if (*pos != '"')
			return false;
		pos++;

		if (*pos == '"')
		{
			board[i] = '\0';
			pos++;
		}
		else if (*pos == 'X' || *pos == 'O')
		{
			board[i] = *pos;
			pos++;
			if (*pos != '"')
				return false;
			pos++;
		}
		else
		{
			return false;
		}
	}
	return true;
}

static bool load_room(const char *room_id, game_room_t *room)
{
	char filepath[PATH_BUF];
	build_filepath(filepath, sizeof(filepath), room_id);

	FILE *const f = fopen(filepath, "r");
	if (!f)
		return false;

	char buf[FILE_BUF] = {};
	const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';

	if (!parse_board(buf, room->board))
		return false;

	char turn_str[4] = {};
	if (!json_extract_string(buf, "turn", turn_str, sizeof(turn_str)))
		return false;
	room->turn = char_to_symbol(turn_str[0]);

	/* Parsear players — busca dentro do objeto "players":{...} */
	const char *players_start = strstr(buf, "\"players\":{");
	if (!players_start)
		return false;

	char player_section[256] = {};
	const char *brace = strchr(players_start, '{');
	if (!brace)
		return false;

	const char *brace_end = strchr(brace, '}');
	if (!brace_end)
		return false;

	size_t section_len = (size_t)(brace_end - brace + 1);
	if (section_len >= sizeof(player_section))
		return false;
	memcpy(player_section, brace, section_len);
	player_section[section_len] = '\0';

	json_extract_string(player_section, "X", room->player_x, sizeof(room->player_x));

	bool o_is_null = false;
	json_extract_nullable_string(
	    player_section, "O", room->player_o, sizeof(room->player_o), &o_is_null
	);
	if (o_is_null)
		room->player_o[0] = '\0';

	/* winner */
	bool winner_null = false;
	char winner_str[16] = {};
	json_extract_nullable_string(buf, "winner", winner_str, sizeof(winner_str), &winner_null);

	if (winner_null)
	{
		room->winner = SYMBOL_NONE;
		room->is_draw = false;
	}
	else if (strcmp(winner_str, "Empate") == 0)
	{
		room->winner = SYMBOL_NONE;
		room->is_draw = true;
	}
	else
	{
		room->winner = char_to_symbol(winner_str[0]);
		room->is_draw = false;
	}

	long long ts = 0;
	json_extract_long(buf, "createdAt", &ts);
	room->created_at = (time_t)ts;

	return true;
}

/* ── Serialização JSON ───────────────────────────────────── */

static void serialize_room(const game_room_t *room, char *out, size_t out_size)
{
	char board_json[128];
	size_t off = 0;

	off += (size_t)snprintf(board_json + off, sizeof(board_json) - off, "[");

	for (size_t i = 0; i < BOARD_SIZE; i++)
	{
		const char *sep = (i > 0) ? "," : "";
		if (room->board[i] == '\0')
		{
			off += (size_t)snprintf(board_json + off, sizeof(board_json) - off, "%s\"\"", sep);
		}
		else
		{
			off += (size_t)snprintf(
			    board_json + off, sizeof(board_json) - off, "%s\"%c\"", sep, room->board[i]
			);
		}
	}
	snprintf(board_json + off, sizeof(board_json) - off, "]");

	const char *turn_str = (room->turn == SYMBOL_X) ? "X" : "O";

	char player_o_json[64];
	if (room->player_o[0] == '\0')
	{
		snprintf(player_o_json, sizeof(player_o_json), "null");
	}
	else
	{
		snprintf(player_o_json, sizeof(player_o_json), "\"%.32s\"", room->player_o);
	}

	char winner_json[32];
	if (room->is_draw)
	{
		snprintf(winner_json, sizeof(winner_json), "\"Empate\"");
	}
	else if (room->winner != SYMBOL_NONE)
	{
		snprintf(winner_json, sizeof(winner_json), "\"%c\"", symbol_to_char(room->winner));
	}
	else
	{
		snprintf(winner_json, sizeof(winner_json), "null");
	}

	snprintf(
	    out,
	    out_size,
	    "{\"board\":%s,"
	    "\"turn\":\"%s\","
	    "\"players\":{\"X\":\"%.32s\",\"O\":%s},"
	    "\"winner\":%s,"
	    "\"createdAt\":%lld}",
	    board_json,
	    turn_str,
	    room->player_x,
	    player_o_json,
	    winner_json,
	    (long long)room->created_at
	);
}

static bool save_room(const char *room_id, const game_room_t *room)
{
	char filepath[PATH_BUF];
	build_filepath(filepath, sizeof(filepath), room_id);

	FILE *const f = fopen(filepath, "w");
	if (!f)
		return false;

	char json[FILE_BUF];
	serialize_room(room, json, sizeof(json));
	fputs(json, f);
	fclose(f);
	return true;
}

/* ── Verificação de vitória ──────────────────────────────── */

static void check_winner(game_room_t *room)
{
	for (size_t i = 0; i < 8; i++)
	{
		const size_t a = WIN_COMBOS[i][0];
		const size_t b = WIN_COMBOS[i][1];
		const size_t c = WIN_COMBOS[i][2];

		if (room->board[a] != '\0' && room->board[a] == room->board[b] &&
		    room->board[a] == room->board[c])
		{
			room->winner = char_to_symbol(room->board[a]);
			return;
		}
	}

	bool full = true;
	for (size_t i = 0; i < BOARD_SIZE; i++)
	{
		if (room->board[i] == '\0')
		{
			full = false;
			break;
		}
	}

	if (full)
	{
		room->is_draw = true;
	}
}

/* ── Room: estado terminado? ─────────────────────────────── */

static bool room_is_finished(const game_room_t *room)
{
	return room->winner != SYMBOL_NONE || room->is_draw;
}

/* ── Interface Pública ───────────────────────────────────── */

void game_ensure_data_dir(void)
{
	struct stat st = {};
	if (stat(DATA_DIR, &st) == -1)
	{
		mkdir(DATA_DIR, 0755);
	}
}

void game_purge_expired(void)
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

		char filepath[PATH_BUF];
		snprintf(filepath, sizeof(filepath), "%s%s", DATA_DIR, ent->d_name);

		/* Extrair room_id do nome do arquivo */
		const char *id_start = ent->d_name + ROOM_PREFIX_LEN;
		const char *dot = strstr(id_start, ROOM_SUFFIX);
		if (!dot)
			continue;

		char room_id[ROOM_ID_SIZE] = {};
		size_t id_len = (size_t)(dot - id_start);
		if (id_len >= sizeof(room_id))
			continue;
		memcpy(room_id, id_start, id_len);
		room_id[id_len] = '\0';

		game_room_t room = {};
		if (!load_room(room_id, &room))
		{
			/* Arquivo corrompido: remover */
			remove(filepath);
			continue;
		}

		const bool expired = (now - room.created_at > ROOM_DURATION_SECS);
		const bool finished_and_grace = room_is_finished(&room) &&
		                                (now - room.created_at > ROOM_GRACE_PERIOD_SECS);

		if (expired || finished_and_grace)
		{
			remove(filepath);
		}
	}
	closedir(dir);
}

size_t game_active_count(void)
{
	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
		return 0;

	size_t count = 0;
	struct dirent *ent;

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strncmp(ent->d_name, ROOM_PREFIX, ROOM_PREFIX_LEN) != 0)
			continue;

		const char *id_start = ent->d_name + ROOM_PREFIX_LEN;
		const char *dot = strstr(id_start, ROOM_SUFFIX);
		if (!dot)
			continue;

		char room_id[ROOM_ID_SIZE] = {};
		size_t id_len = (size_t)(dot - id_start);
		if (id_len >= sizeof(room_id))
			continue;
		memcpy(room_id, id_start, id_len);

		game_room_t room = {};
		if (load_room(room_id, &room) && !room_is_finished(&room))
		{
			count++;
		}
	}
	closedir(dir);
	return count;
}

game_error_t game_create_room(
    const char *room_id, const char *player_name, char *out_json, size_t json_size
)
{
	game_ensure_data_dir();
	game_purge_expired();

	if (game_active_count() >= MAX_ROOMS)
	{
		return GAME_ERR_MAX_ROOMS;
	}

	/* Verificar se já existe */
	char filepath[PATH_BUF];
	build_filepath(filepath, sizeof(filepath), room_id);

	struct stat st;
	if (stat(filepath, &st) == 0)
	{
		return GAME_ERR_ROOM_FULL;
	}

	game_room_t room = {};
	memset(room.board, '\0', BOARD_SIZE);
	room.turn = SYMBOL_X;
	room.winner = SYMBOL_NONE;
	room.is_draw = false;
	room.created_at = time(nullptr);

	const size_t name_len = strlen(player_name);
	const size_t copy_len = (name_len < MAX_PLAYER_NAME - 1) ? name_len : MAX_PLAYER_NAME - 1;
	memcpy(room.player_x, player_name, copy_len);
	room.player_x[copy_len] = '\0';
	room.player_o[0] = '\0';

	if (!save_room(room_id, &room))
		return GAME_ERR_IO;

	if (out_json)
	{
		serialize_room(&room, out_json, json_size);
	}

	return GAME_OK;
}

game_error_t game_join_room(
    const char *room_id, const char *player_name, game_symbol_t *out_symbol
)
{
	game_room_t room = {};
	if (!load_room(room_id, &room))
		return GAME_ERR_ROOM_NOT_FOUND;

	/* Se já é o jogador X, retorna X */
	if (strcmp(room.player_x, player_name) == 0)
	{
		*out_symbol = SYMBOL_X;
		return GAME_OK;
	}

	/* Se já é o jogador O, retorna O */
	if (room.player_o[0] != '\0' && strcmp(room.player_o, player_name) == 0)
	{
		*out_symbol = SYMBOL_O;
		return GAME_OK;
	}

	/* Se O está vazio, atribuir */
	if (room.player_o[0] == '\0')
	{
		const size_t name_len = strlen(player_name);
		const size_t copy_len = (name_len < MAX_PLAYER_NAME - 1) ? name_len
		                                                         : MAX_PLAYER_NAME - 1;
		memcpy(room.player_o, player_name, copy_len);
		room.player_o[copy_len] = '\0';

		if (!save_room(room_id, &room))
			return GAME_ERR_IO;

		*out_symbol = SYMBOL_O;
		return GAME_OK;
	}

	return GAME_ERR_ROOM_FULL;
}

game_error_t game_make_move(
    const char *room_id, const char *player_name, const game_symbol_t symbol,
    const size_t cell_index
)
{
	if (cell_index >= BOARD_SIZE)
		return GAME_ERR_INVALID_CELL;
	if (symbol != SYMBOL_X && symbol != SYMBOL_O)
		return GAME_ERR_INVALID_SYMBOL;

	game_room_t room = {};
	if (!load_room(room_id, &room))
		return GAME_ERR_ROOM_NOT_FOUND;

	if (room_is_finished(&room))
		return GAME_ERR_GAME_OVER;
	if (room.turn != symbol)
		return GAME_ERR_NOT_YOUR_TURN;

	/* Verificar que o jogador corresponde ao símbolo */
	if (symbol == SYMBOL_X && strcmp(room.player_x, player_name) != 0)
	{
		return GAME_ERR_INVALID_SYMBOL;
	}
	if (symbol == SYMBOL_O && strcmp(room.player_o, player_name) != 0)
	{
		return GAME_ERR_INVALID_SYMBOL;
	}

	if (room.board[cell_index] != '\0')
		return GAME_ERR_CELL_OCCUPIED;

	room.board[cell_index] = symbol_to_char(symbol);

	check_winner(&room);

	if (!room_is_finished(&room))
	{
		room.turn = (symbol == SYMBOL_X) ? SYMBOL_O : SYMBOL_X;
	}

	if (!save_room(room_id, &room))
		return GAME_ERR_IO;

	return GAME_OK;
}

game_error_t game_get_state(const char *room_id, char *out_json, const size_t json_size)
{
	game_room_t room = {};
	if (!load_room(room_id, &room))
		return GAME_ERR_ROOM_NOT_FOUND;

	serialize_room(&room, out_json, json_size);
	return GAME_OK;
}

game_error_t game_list_rooms(char *out_json, const size_t json_size)
{
	game_purge_expired();

	DIR *const dir = opendir(DATA_DIR);
	if (!dir)
	{
		snprintf(out_json, json_size, "[]");
		return GAME_OK;
	}

	size_t off = 0;
	off += (size_t)snprintf(out_json + off, json_size - off, "[");

	bool first = true;
	struct dirent *ent;

	while ((ent = readdir(dir)) != nullptr)
	{
		if (strncmp(ent->d_name, ROOM_PREFIX, ROOM_PREFIX_LEN) != 0)
			continue;

		const char *id_start = ent->d_name + ROOM_PREFIX_LEN;
		const char *dot = strstr(id_start, ROOM_SUFFIX);
		if (!dot)
			continue;

		char room_id[ROOM_ID_SIZE] = {};
		size_t id_len = (size_t)(dot - id_start);
		if (id_len >= sizeof(room_id))
			continue;
		memcpy(room_id, id_start, id_len);

		game_room_t room = {};
		if (!load_room(room_id, &room))
			continue;
		if (room_is_finished(&room))
			continue;

		const char *sep = first ? "" : ",";
		first = false;

		char player_o_json[64];
		if (room.player_o[0] == '\0')
		{
			snprintf(player_o_json, sizeof(player_o_json), "null");
		}
		else
		{
			snprintf(player_o_json, sizeof(player_o_json), "\"%.32s\"", room.player_o);
		}

		const int written = snprintf(
		    out_json + off,
		    json_size - off,
		    "%s{\"id\":\"%.6s\",\"player_x\":\"%.32s\",\"player_o\":%s}",
		    sep,
		    room_id,
		    room.player_x,
		    player_o_json
		);

		if (written < 0 || (size_t)written >= (json_size - off))
			break;
		off += (size_t)written;
	}
	closedir(dir);

	snprintf(out_json + off, json_size - off, "]");
	return GAME_OK;
}

const char *game_error_string(const game_error_t err)
{
	switch (err)
	{
	case GAME_OK:
		return "OK";
	case GAME_ERR_NOT_YOUR_TURN:
		return "Não é seu turno.";
	case GAME_ERR_CELL_OCCUPIED:
		return "Célula já ocupada.";
	case GAME_ERR_GAME_OVER:
		return "O jogo já terminou.";
	case GAME_ERR_INVALID_CELL:
		return "Célula inválida.";
	case GAME_ERR_INVALID_SYMBOL:
		return "Símbolo inválido ou jogador incorreto.";
	case GAME_ERR_ROOM_FULL:
		return "Sala cheia.";
	case GAME_ERR_ROOM_NOT_FOUND:
		return "Sala não encontrada.";
	case GAME_ERR_MAX_ROOMS:
		return "Limite de salas atingido.";
	case GAME_ERR_PARSE:
		return "Erro ao interpretar estado da sala.";
	case GAME_ERR_IO:
		return "Erro de leitura/escrita.";
	}
	return "Erro desconhecido.";
}
