#include "board_imu.h"

#include "board.h"

#include <string.h>

#define IMU_RX_BUFFER_SIZE  128U
#define WT901_PACKET_SIZE   11U

#define WT901_HEADER        0x55U
#define WT901_TYPE_GYRO     0x52U
#define WT901_TYPE_ANGLE    0x53U

static uint8_t rx_buffer[IMU_RX_BUFFER_SIZE];
static uint8_t packet[WT901_PACKET_SIZE];
static uint8_t packet_index;

static board_imu_data_t imu_data;

static int16_t little_endian_i16(const uint8_t *data)
{
    return (int16_t)(
        (uint16_t)data[0] |
        ((uint16_t)data[1] << 8U));
}

static void parse_packet(const uint8_t *data)
{
    uint8_t checksum = 0U;

    for (uint8_t index = 0U; index < 10U; index++) {
        checksum += data[index];
    }

    if (data[0] != WT901_HEADER || checksum != data[10]) {
        return;
    }

    if (data[1] == WT901_TYPE_ANGLE) {
        imu_data.roll_deg =
            ((float)little_endian_i16(&data[2]) / 32768.0f) * 180.0f;

        imu_data.pitch_deg =
            ((float)little_endian_i16(&data[4]) / 32768.0f) * 180.0f;

        imu_data.yaw_deg =
            ((float)little_endian_i16(&data[6]) / 32768.0f) * 180.0f;

        imu_data.last_update_ms = HAL_GetTick();
        imu_data.valid = true;
    } else if (data[1] == WT901_TYPE_GYRO) {
        imu_data.p_dps =
            ((float)little_endian_i16(&data[2]) / 32768.0f) * 2000.0f;

        imu_data.q_dps =
            ((float)little_endian_i16(&data[4]) / 32768.0f) * 2000.0f;

        imu_data.r_dps =
            ((float)little_endian_i16(&data[6]) / 32768.0f) * 2000.0f;
    }
}

bool board_imu_init(void)
{
    memset(&imu_data, 0, sizeof(imu_data));
    packet_index = 0U;

    return true;
}

void board_imu_start_rx(void)
{
    (void)HAL_UARTEx_ReceiveToIdle_DMA(
        &huart3,
        rx_buffer,
        IMU_RX_BUFFER_SIZE);

    if (huart3.hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }
}

void board_imu_on_rx_event(uint16_t size)
{
    for (uint16_t index = 0U; index < size; index++) {
        const uint8_t byte = rx_buffer[index];

        if (packet_index == 0U && byte != WT901_HEADER) {
            continue;
        }

        packet[packet_index++] = byte;

        if (packet_index == WT901_PACKET_SIZE) {
            parse_packet(packet);
            packet_index = 0U;
        }
    }

    board_imu_start_rx();
}

board_imu_data_t board_imu_get(void)
{
    board_imu_data_t result = imu_data;

    if ((HAL_GetTick() - result.last_update_ms) > 1000U) {
        result.valid = false;
    }

    return result;
}
