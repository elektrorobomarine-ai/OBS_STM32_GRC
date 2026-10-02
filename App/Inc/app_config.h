#pragma once
#include <stdint.h>

#define OBS_FW_VERSION_MAJOR 1U
#define OBS_FW_VERSION_MINOR 0U
#define OBS_FW_VERSION_PATCH 0U
#define OBS_FW_VERSION_STR   "1.0.0"
#define OBS_FW_TITLE         "ROBOMARINE OBS STM32H743"

#define OBS_ADC_CHANNELS             4U
#define OBS_ADC_SAMPLE_RATE_HZ       1000U
#define OBS_ADC_FRAMES_PER_RECORD    128U
#define OBS_ADC_WORDS_PER_RECORD     (OBS_ADC_CHANNELS * OBS_ADC_FRAMES_PER_RECORD)
#define OBS_ADC_BYTES_PER_RECORD     (OBS_ADC_WORDS_PER_RECORD * 4U)
#define OBS_ADC_MCLK_HZ              32768000U

#define OBS_TCP_COMMAND_PORT         54300U
#define OBS_TCP_DATA_PORT            54301U
#define OBS_TCP_MAX_CLIENTS          2U
#define OBS_NET_STATIC_IP0           192U
#define OBS_NET_STATIC_IP1           168U
#define OBS_NET_STATIC_IP2           3U
#define OBS_NET_STATIC_IP3           101U

#define OBS_IMU_BAUDRATE             115200U
#define OBS_SENSOR_PERIOD_MS         20U
#define OBS_TELEM_FAST_PERIOD_MS     40U
#define OBS_TELEM_MEDIUM_PERIOD_MS   200U
#define OBS_TELEM_SLOW_PERIOD_MS     1000U

#define OBS_USB_BATCH_RECORDS        16U
#define OBS_USB_LOG_ROTATE_SEC       10U
#define OBS_USB_SYNC_RECORDS         16U
#define OBS_USB_DRIVE_PATH           "0:"

#define OBS_RELAY_ACTIVE_HIGH        1U
#define OBS_LEAK_THRESHOLD_RAW       2000U
#define OBS_VBAT_ADC_FULL_SCALE      65535.0f
#define OBS_VBAT_ADC_REF_V           3.3f
#define OBS_VBAT_DIVIDER_RATIO       4.030303f /* TODO bench-calibrate */

#define OBS_DEPTH_SENSOR_MS5837      1U
#define OBS_MS5837_ADDR_8BIT         0xECU
#define OBS_MS5837_MODEL_30BA        1U
