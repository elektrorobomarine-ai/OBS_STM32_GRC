#include "board_depth.h"

#include "app_config.h"
#include "board.h"

#include <string.h>

#define MS5837_CMD_RESET           0x1EU
#define MS5837_CMD_ADC_READ        0x00U
#define MS5837_CMD_PROM_BASE       0xA0U
#define MS5837_CMD_CONVERT_D1_OSR  0x4AU
#define MS5837_CMD_CONVERT_D2_OSR  0x5AU

static uint16_t calibration[8];
static board_depth_data_t last_data;

static bool ms5837_send_command(uint8_t command)
{
    return HAL_I2C_Master_Transmit(
               &hi2c1,
               OBS_MS5837_ADDR_8BIT,
               &command,
               1U,
               50U) == HAL_OK;
}

static uint8_t ms5837_crc4(const uint16_t *prom)
{
    uint16_t remainder = 0U;
    uint16_t prom_copy[8];

    memcpy(prom_copy, prom, sizeof(prom_copy));

    prom_copy[0] &= 0x0FFFU;
    prom_copy[7] = 0U;

    for (uint8_t count = 0U; count < 16U; count++) {
        if ((count & 1U) != 0U) {
            remainder ^= (prom_copy[count >> 1U] & 0x00FFU);
        } else {
            remainder ^= (prom_copy[count >> 1U] >> 8U);
        }

        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((remainder & 0x8000U) != 0U) {
                remainder = (uint16_t)((remainder << 1U) ^ 0x3000U);
            } else {
                remainder <<= 1U;
            }
        }
    }

    return (uint8_t)((remainder >> 12U) & 0x0FU);
}

static bool ms5837_read_adc(uint8_t conversion_command, uint32_t *value)
{
    uint8_t bytes[3];

    if (value == NULL) {
        return false;
    }

    if (!ms5837_send_command(conversion_command)) {
        return false;
    }

    HAL_Delay(10U);

    if (!ms5837_send_command(MS5837_CMD_ADC_READ)) {
        return false;
    }

    if (HAL_I2C_Master_Receive(
            &hi2c1,
            OBS_MS5837_ADDR_8BIT,
            bytes,
            sizeof(bytes),
            50U) != HAL_OK) {
        return false;
    }

    *value =
        ((uint32_t)bytes[0] << 16U) |
        ((uint32_t)bytes[1] << 8U) |
        (uint32_t)bytes[2];

    return true;
}

bool board_depth_init(void)
{
    memset(&last_data, 0, sizeof(last_data));

    if (!ms5837_send_command(MS5837_CMD_RESET)) {
        return false;
    }

    HAL_Delay(20U);

    for (uint8_t index = 0U; index < 8U; index++) {
        uint8_t command =
            (uint8_t)(MS5837_CMD_PROM_BASE + (index * 2U));

        uint8_t bytes[2];

        if (!ms5837_send_command(command)) {
            return false;
        }

        if (HAL_I2C_Master_Receive(
                &hi2c1,
                OBS_MS5837_ADDR_8BIT,
                bytes,
                sizeof(bytes),
                50U) != HAL_OK) {
            return false;
        }

        calibration[index] =
            ((uint16_t)bytes[0] << 8U) |
            (uint16_t)bytes[1];
    }

    return
        ms5837_crc4(calibration) ==
        ((calibration[0] >> 12U) & 0x0FU);
}

bool board_depth_read(board_depth_data_t *out)
{
    uint32_t d1;
    uint32_t d2;

    if (out == NULL) {
        return false;
    }

    if (!ms5837_read_adc(MS5837_CMD_CONVERT_D1_OSR, &d1)) {
        return false;
    }

    if (!ms5837_read_adc(MS5837_CMD_CONVERT_D2_OSR, &d2)) {
        return false;
    }

    const int32_t dT =
        (int32_t)d2 -
        ((int32_t)calibration[5] << 8U);

    const int64_t sens =
        (32768LL * calibration[1]) +
        (((int64_t)calibration[3] * dT) / 256LL);

    const int64_t off =
        (65536LL * calibration[2]) +
        (((int64_t)calibration[4] * dT) / 128LL);

    const int32_t temp_centi_c =
        (int32_t)(
            2000LL +
            (((int64_t)dT * calibration[6]) / 8388608LL));

    /* MS5837-30BA result is in 0.1 mbar. */
    const int32_t pressure_deci_mbar =
        (int32_t)(
            ((((int64_t)d1 * sens) / 2097152LL) - off) /
            8192LL);

    const float pressure_mbar =
        (float)pressure_deci_mbar / 10.0f;

    float depth_m =
        (pressure_mbar - 1013.25f) * 0.01019716213f;

    if (depth_m < 0.0f) {
        depth_m = 0.0f;
    }

    const uint32_t now = HAL_GetTick();
    float rate_mps = 0.0f;

    if (last_data.valid && now != last_data.last_update_ms) {
        rate_mps =
            (depth_m - last_data.depth_m) *
            1000.0f /
            (float)(now - last_data.last_update_ms);
    }

    last_data.depth_m = depth_m;
    last_data.temperature_c = (float)temp_centi_c / 100.0f;
    last_data.pressure_mbar = pressure_mbar;
    last_data.rate_mps = rate_mps;
    last_data.last_update_ms = now;
    last_data.valid = true;

    *out = last_data;

    return true;
}
