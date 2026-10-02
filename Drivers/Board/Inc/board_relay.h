#pragma once
#include <stdbool.h>

typedef enum {
    BOARD_RELAY_1 = 0,
    BOARD_RELAY_2 = 1
} board_relay_id_t;

void board_relay_init(void);
void board_relay_set(board_relay_id_t id, bool on);
bool board_relay_get(board_relay_id_t id);
