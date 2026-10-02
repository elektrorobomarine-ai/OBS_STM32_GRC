#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint8_t obs_nmea_checksum(const char *body);

size_t obs_nmea_build(
    char *dst,
    size_t capacity,
    const char *body);

bool obs_nmea_validate(
    char *line,
    char **body_out);
