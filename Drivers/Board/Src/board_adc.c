#include "board_adc.h"

#include "ad7768_regs.h"
#include "board.h"

#include "cmsis_os2.h"

#include <string.h>

#define DMA_TOTAL_FRAMES  (OBS_ADC_FRAMES_PER_RECORD * 2U)
#define DMA_WORDS         (DMA_TOTAL_FRAMES * OBS_ADC_CHANNELS)

#define ADC_DMA_EVENT_HALF  0U
#define ADC_DMA_EVENT_FULL  1U

#if defined(__GNUC__)
__attribute__((aligned(32), section(".sai_dma_buffer")))
#endif
static uint32_t dma_buffer[DMA_WORDS];

static osMessageQueueId_t dma_event_queue;
static board_adc_diag_t diagnostics;

static uint64_t frame_index;
static uint32_t rate_tick_ms;
static uint64_t rate_frame_index;

static void adc_cs_set(bool active)
{
    HAL_GPIO_WritePin(
        ADC_CS_GPIO_Port,
        ADC_CS_Pin,
        active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static bool spi_transfer_16(
    uint8_t tx0,
    uint8_t tx1,
    uint8_t *rx0,
    uint8_t *rx1)
{
    uint8_t tx[2] = {tx0, tx1};
    uint8_t rx[2] = {0U, 0U};

    adc_cs_set(true);

    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(
            &hspi2,
            tx,
            rx,
            sizeof(tx),
            100U);

    adc_cs_set(false);

    if (rx0 != NULL) {
        *rx0 = rx[0];
    }

    if (rx1 != NULL) {
        *rx1 = rx[1];
    }

    return status == HAL_OK;
}

bool board_adc_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];

    if ((reg & 0x80U) != 0U) {
        return false;
    }

    tx[0] = reg;
    tx[1] = value;

    adc_cs_set(true);

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(
            &hspi2,
            tx,
            sizeof(tx),
            100U);

    adc_cs_set(false);

    return status == HAL_OK;
}

bool board_adc_read_reg(uint8_t reg, uint8_t *value)
{
    uint8_t high_byte;
    uint8_t low_byte;
    uint8_t command;

    if (value == NULL || (reg & 0x80U) != 0U) {
        return false;
    }

    command = (uint8_t)(0x80U | reg);

    /*
     * AD7768 register read is pipelined. Two identical 16-bit frames are
     * issued and the second response contains the requested register value.
     */
    if (!spi_transfer_16(
            command,
            0U,
            &high_byte,
            &low_byte)) {
        return false;
    }

    if (!spi_transfer_16(
            command,
            0U,
            &high_byte,
            &low_byte)) {
        return false;
    }

    *value = low_byte;

    return true;
}

static bool write_verify_reg(
    uint8_t reg,
    uint8_t value,
    uint8_t verify_mask)
{
    uint8_t readback = 0U;

    if (!board_adc_write_reg(reg, value)) {
        return false;
    }

    if (!board_adc_read_reg(reg, &readback)) {
        return false;
    }

    return
        (readback & verify_mask) ==
        (value & verify_mask);
}

static void update_word_diagnostics(
    uint32_t word,
    uint32_t expected_channel)
{
    const uint8_t header = (uint8_t)(word >> 24U);

    if ((header & AD7768_HDR_CHANNEL_ID_MASK) != expected_channel) {
        diagnostics.channel_id_errors++;
    }

    if ((header & AD7768_HDR_ERROR_FLAGGED) != 0U) {
        diagnostics.header_error_flagged++;
    }

    if ((header & AD7768_HDR_FILTER_NOT_SETTLED) != 0U) {
        diagnostics.filter_not_settled++;
    }

    if ((header & AD7768_HDR_REPEATED_DATA) != 0U) {
        diagnostics.repeated_data++;
    }

    if ((header & AD7768_HDR_FILTER_SATURATED) != 0U) {
        diagnostics.filter_saturated++;
    }
}

bool board_adc_init(void)
{
    memset(&diagnostics, 0, sizeof(diagnostics));

    frame_index = 0U;

    dma_event_queue =
        osMessageQueueNew(
            8U,
            sizeof(uint8_t),
            NULL);

    if (dma_event_queue == NULL) {
        return false;
    }

    HAL_GPIO_WritePin(
        ADC_CS_GPIO_Port,
        ADC_CS_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        ADC_SYNC_IN_GPIO_Port,
        ADC_SYNC_IN_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        ADC_RESET_GPIO_Port,
        ADC_RESET_Pin,
        GPIO_PIN_RESET);

    HAL_Delay(2U);

    HAL_GPIO_WritePin(
        ADC_RESET_GPIO_Port,
        ADC_RESET_Pin,
        GPIO_PIN_SET);

    HAL_Delay(20U);

    /*
     * OBS fixed acquisition profile:
     *   MCLK       = 32.768 MHz
     *   MCLK div   = 32
     *   Decimation = 1024
     *   ODR        = 1 kSPS
     *   Filter     = wideband
     */
    if (!write_verify_reg(
            AD7768_REG_CHANNEL_STANDBY,
            0x00U,
            0x0FU)) {
        return false;
    }

    if (!write_verify_reg(
            AD7768_REG_CHANNEL_MODE_A,
            (uint8_t)(
                AD7768_FILTER_WIDEBAND |
                AD7768_DEC_1024),
            0x0FU)) {
        return false;
    }

    if (!write_verify_reg(
            AD7768_REG_CHANNEL_MODE_SELECT,
            0x00U,
            0x0FU)) {
        return false;
    }

    if (!write_verify_reg(
            AD7768_REG_POWER_MODE,
            (uint8_t)(
                AD7768_POWER_LOW |
                AD7768_MCLK_DIV_32),
            0x3BU)) {
        return false;
    }

    if (!write_verify_reg(
            AD7768_REG_INTERFACE_CONFIG,
            (uint8_t)(
                AD7768_CRC_NONE |
                AD7768_DCLK_DIV_1),
            0x0FU)) {
        return false;
    }

    (void)board_adc_read_reg(
        AD7768_REG_DEVICE_STATUS,
        &diagnostics.device_status);

    (void)board_adc_read_reg(
        AD7768_REG_REVISION_ID,
        &diagnostics.revision_id);

    return true;
}

bool board_adc_start(void)
{
    if (diagnostics.running) {
        return true;
    }

    rate_tick_ms = HAL_GetTick();
    rate_frame_index = frame_index;

    if (HAL_TIM_PWM_Start(
            &htim1,
            TIM_CHANNEL_1) != HAL_OK) {
        return false;
    }

    if (HAL_SAI_Receive_DMA(
            &hsai_BlockA1,
            (uint8_t *)dma_buffer,
            DMA_WORDS) != HAL_OK) {
        (void)HAL_TIM_PWM_Stop(
            &htim1,
            TIM_CHANNEL_1);

        return false;
    }

    diagnostics.running = true;

    return true;
}

void board_adc_stop(void)
{
    (void)HAL_SAI_DMAStop(&hsai_BlockA1);
    (void)HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

    diagnostics.running = false;
}

void board_adc_on_dma_half_complete(void)
{
    uint8_t event = ADC_DMA_EVENT_HALF;

    if (osMessageQueuePut(
            dma_event_queue,
            &event,
            0U,
            0U) != osOK) {
        diagnostics.blocks_dropped++;
    }
}

void board_adc_on_dma_complete(void)
{
    uint8_t event = ADC_DMA_EVENT_FULL;

    if (osMessageQueuePut(
            dma_event_queue,
            &event,
            0U,
            0U) != osOK) {
        diagnostics.blocks_dropped++;
    }
}

void board_adc_on_sai_error(void)
{
    diagnostics.sai_errors++;
}

bool board_adc_get_block(
    board_adc_block_t *block,
    uint32_t timeout_ms)
{
    uint8_t event;
    uint32_t word_offset;

    if (block == NULL) {
        return false;
    }

    if (osMessageQueueGet(
            dma_event_queue,
            &event,
            NULL,
            timeout_ms) != osOK) {
        return false;
    }

    word_offset =
        (event == ADC_DMA_EVENT_FULL)
            ? OBS_ADC_WORDS_PER_RECORD
            : 0U;

#if (__DCACHE_PRESENT == 1U)
    SCB_InvalidateDCache_by_Addr(
        (uint32_t *)&dma_buffer[word_offset],
        OBS_ADC_BYTES_PER_RECORD);
#endif

    block->first_frame_index = frame_index;
    block->frame_count = OBS_ADC_FRAMES_PER_RECORD;

    memcpy(
        block->raw_words,
        &dma_buffer[word_offset],
        OBS_ADC_BYTES_PER_RECORD);

    for (
        uint32_t frame = 0U;
        frame < OBS_ADC_FRAMES_PER_RECORD;
        frame++) {
        for (
            uint32_t channel = 0U;
            channel < OBS_ADC_CHANNELS;
            channel++) {
            const uint32_t word_index =
                (frame * OBS_ADC_CHANNELS) +
                channel;

            update_word_diagnostics(
                block->raw_words[word_index],
                channel);
        }
    }

    frame_index += OBS_ADC_FRAMES_PER_RECORD;

    diagnostics.frames_received +=
        OBS_ADC_FRAMES_PER_RECORD;

    diagnostics.blocks_produced++;

    const uint32_t now = HAL_GetTick();
    const uint32_t elapsed_ms = now - rate_tick_ms;

    if (elapsed_ms >= 1000U) {
        const uint64_t frames_elapsed =
            frame_index - rate_frame_index;

        diagnostics.measured_rate_millihz =
            (uint32_t)(
                (frames_elapsed * 1000000ULL) /
                elapsed_ms);

        rate_tick_ms = now;
        rate_frame_index = frame_index;
    }

    return true;
}

void board_adc_get_diagnostics(board_adc_diag_t *out)
{
    if (out != NULL) {
        *out = diagnostics;
    }
}
