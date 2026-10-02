#pragma once

#include <stdbool.h>

#include "board_adc.h"

bool obs_network_init(void);
void obs_network_task(void *argument);
bool obs_network_submit_adc(const board_adc_block_t *block);
