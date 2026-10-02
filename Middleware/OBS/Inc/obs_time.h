#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t year;

    uint8_t month;
    uint8_t date;
    uint8_t weekday;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint16_t ms;
} obs_utc_t;

void obs_time_init(void);

bool obs_time_get_utc(obs_utc_t *time);
bool obs_time_set_utc(const obs_utc_t *time);

uint64_t obs_time_now_us_utc(void);
