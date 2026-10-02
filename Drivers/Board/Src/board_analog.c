#include "board_analog.h"

#include "app_config.h"
#include "board.h"

static uint32_t adc_raw[2];

bool board_analog_init(void)
{
    return HAL_ADC_Start_DMA(&hadc1, adc_raw, 2U) == HAL_OK;
}

bool board_analog_read(board_analog_data_t *out)
{
    uint16_t leak_raw;
    uint16_t vbat_raw;

    if (out == NULL) {
        return false;
    }

    leak_raw = (uint16_t)(adc_raw[0] & 0xFFFFU);
    vbat_raw = (uint16_t)(adc_raw[1] & 0xFFFFU);

    out->leak_raw = leak_raw;
    out->vbat_raw = vbat_raw;

    out->leak_v =
        (float)leak_raw *
        (OBS_VBAT_ADC_REF_V / OBS_VBAT_ADC_FULL_SCALE);

    out->battery_v =
        (float)vbat_raw *
        (OBS_VBAT_ADC_REF_V / OBS_VBAT_ADC_FULL_SCALE) *
        OBS_VBAT_DIVIDER_RATIO;

    out->leak_alarm = (leak_raw >= OBS_LEAK_THRESHOLD_RAW);
    out->valid = true;

    return true;
}
