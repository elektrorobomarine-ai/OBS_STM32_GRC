#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t timestamp_us_utc;
    int32_t adc[4];
} obs_sample_t;

typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float depth_m;
    float temperature_c;
    float battery_v;
    float leak_v;
    bool leak_alarm;
} obs_telemetry_t;
