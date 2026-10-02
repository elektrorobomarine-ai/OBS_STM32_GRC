#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float depth_m;
    float temperature_c;
    float pressure_mbar;
    float rate_mps;

    uint32_t last_update_ms;
    bool valid;
} board_depth_data_t;

bool board_depth_init(void);
bool board_depth_read(board_depth_data_t *out);
