#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t leak_raw;
    uint16_t vbat_raw;

    float leak_v;
    float battery_v;

    bool leak_alarm;
    bool valid;
} board_analog_data_t;

bool board_analog_init(void);
bool board_analog_read(board_analog_data_t *out);
