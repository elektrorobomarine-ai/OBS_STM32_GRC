#pragma once
#include <stdint.h>

/* AD7768-4 SPI register map */
#define AD7768_REG_CHANNEL_STANDBY       0x00U
#define AD7768_REG_CHANNEL_MODE_A        0x01U
#define AD7768_REG_CHANNEL_MODE_B        0x02U
#define AD7768_REG_CHANNEL_MODE_SELECT   0x03U
#define AD7768_REG_POWER_MODE            0x04U
#define AD7768_REG_GENERAL_CONFIG        0x05U
#define AD7768_REG_DATA_CONTROL          0x06U
#define AD7768_REG_INTERFACE_CONFIG      0x07U
#define AD7768_REG_BIST_CONTROL          0x08U
#define AD7768_REG_DEVICE_STATUS         0x09U
#define AD7768_REG_REVISION_ID           0x0AU

/* Channel mode x, Reg 0x01/0x02 */
#define AD7768_FILTER_WIDEBAND            (0U << 3)
#define AD7768_FILTER_SINC5               (1U << 3)
#define AD7768_DEC_32                     0x00U
#define AD7768_DEC_64                     0x01U
#define AD7768_DEC_128                    0x02U
#define AD7768_DEC_256                    0x03U
#define AD7768_DEC_512                    0x04U
#define AD7768_DEC_1024                   0x05U

/* Power mode Reg 0x04 */
#define AD7768_POWER_LOW                  (0U << 4)
#define AD7768_POWER_MEDIAN               (2U << 4)
#define AD7768_POWER_FAST                 (3U << 4)
#define AD7768_MCLK_DIV_32                0x00U
#define AD7768_MCLK_DIV_8                 0x02U
#define AD7768_MCLK_DIV_4                 0x03U

/* Interface configuration Reg 0x07 */
#define AD7768_CRC_NONE                   (0U << 2)
#define AD7768_DCLK_DIV_8                 0x00U
#define AD7768_DCLK_DIV_4                 0x01U
#define AD7768_DCLK_DIV_2                 0x02U
#define AD7768_DCLK_DIV_1                 0x03U

/* Header byte on every normal 32-bit ADC word */
#define AD7768_HDR_ERROR_FLAGGED          (1U << 7)
#define AD7768_HDR_FILTER_NOT_SETTLED     (1U << 6)
#define AD7768_HDR_REPEATED_DATA          (1U << 5)
#define AD7768_HDR_FILTER_TYPE            (1U << 4)
#define AD7768_HDR_FILTER_SATURATED       (1U << 3)
#define AD7768_HDR_CHANNEL_ID_MASK        0x07U

/* Device status Reg 0x09 */
#define AD7768_STATUS_CHIP_ERROR          (1U << 3)
#define AD7768_STATUS_NO_CLOCK_ERROR      (1U << 2)
#define AD7768_STATUS_RAM_BIST_PASS       (1U << 1)
#define AD7768_STATUS_RAM_BIST_RUNNING    (1U << 0)
