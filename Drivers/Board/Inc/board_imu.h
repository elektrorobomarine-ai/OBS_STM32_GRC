#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;

    float p_dps;
    float q_dps;
    float r_dps;

    uint32_t last_update_ms;
    bool valid;
} board_imu_data_t;

bool board_imu_init(void);
void board_imu_start_rx(void);
void board_imu_on_rx_event(uint16_t size);
board_imu_data_t board_imu_get(void);
