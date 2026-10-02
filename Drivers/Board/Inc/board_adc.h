#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

typedef struct {
    uint64_t first_frame_index;
    uint32_t frame_count;
    uint32_t raw_words[OBS_ADC_WORDS_PER_RECORD]; /* status[31:24] + ADC[23:0] */
} board_adc_block_t;

typedef struct {
    uint64_t frames_received;
    uint64_t blocks_produced;
    uint64_t blocks_dropped;
    uint64_t channel_id_errors;
    uint64_t header_error_flagged;
    uint64_t filter_not_settled;
    uint64_t repeated_data;
    uint64_t filter_saturated;
    uint64_t sai_errors;
    uint32_t measured_rate_millihz;
    uint8_t device_status;
    uint8_t revision_id;
    bool running;
} board_adc_diag_t;

bool board_adc_init(void);
bool board_adc_start(void);
void board_adc_stop(void);
bool board_adc_write_reg(uint8_t reg, uint8_t value);
bool board_adc_read_reg(uint8_t reg, uint8_t *value);
void board_adc_on_dma_half_complete(void);
void board_adc_on_dma_complete(void);
void board_adc_on_sai_error(void);
bool board_adc_get_block(board_adc_block_t *block, uint32_t timeout_ms);
void board_adc_get_diagnostics(board_adc_diag_t *out);
