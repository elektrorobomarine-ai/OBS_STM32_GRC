#include "obs_nmea.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t obs_nmea_checksum(const char *body)
{
    uint8_t checksum = 0U;

    while (*body != '\0' && *body != '*') {
        checksum ^= (uint8_t)(*body);
        body++;
    }

    return checksum;
}

size_t obs_nmea_build(
    char *dst,
    size_t capacity,
    const char *body)
{
    int length;

    if (dst == NULL || body == NULL || capacity < 8U) {
        return 0U;
    }

    length = snprintf(
        dst,
        capacity,
        "$%s*%02X\r\n",
        body,
        obs_nmea_checksum(body));

    if (length <= 0 || (size_t)length >= capacity) {
        return 0U;
    }

    return (size_t)length;
}

bool obs_nmea_validate(
    char *line,
    char **body_out)
{
    char *separator;
    uint8_t expected_checksum;
    uint8_t calculated_checksum;

    if (line == NULL || line[0] != '$') {
        return false;
    }

    separator = strchr(line, '*');

    if (separator == NULL || strlen(separator) < 3U) {
        return false;
    }

    expected_checksum =
        (uint8_t)strtoul(
            separator + 1,
            NULL,
            16);

    *separator = '\0';

    calculated_checksum =
        obs_nmea_checksum(line + 1);

    if (body_out != NULL) {
        *body_out = line + 1;
    }

    return calculated_checksum == expected_checksum;
}
