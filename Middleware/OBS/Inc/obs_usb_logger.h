#pragma once

#include <stdbool.h>

#include "board_adc.h"

bool obs_usb_logger_init(void);
void obs_usb_logger_service(void);

bool obs_usb_logger_start(void);
void obs_usb_logger_stop(void);

bool obs_usb_logger_submit(
    const board_adc_block_t *block);

bool obs_usb_logger_is_active(void);
bool obs_usb_logger_is_mounted(void);
