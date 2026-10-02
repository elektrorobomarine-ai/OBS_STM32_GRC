#pragma once

#include <stdbool.h>

#include "board_analog.h"
#include "board_depth.h"
#include "board_imu.h"

typedef struct {
    board_imu_data_t imu;
    board_depth_data_t depth;
    board_analog_data_t analog;

    bool usb_logging;
    bool usb_mounted;
} obs_state_t;

void obs_state_init(void);

void obs_state_set_imu(board_imu_data_t value);
void obs_state_set_depth(board_depth_data_t value);
void obs_state_set_analog(board_analog_data_t value);

void obs_state_set_usb(
    bool mounted,
    bool logging);

obs_state_t obs_state_get(void);
