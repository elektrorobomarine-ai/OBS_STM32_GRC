#include "app_tasks.h"

#include "app_config.h"
#include "board_adc.h"
#include "board_analog.h"
#include "board_depth.h"
#include "board_imu.h"
#include "obs_network.h"
#include "obs_state.h"
#include "obs_usb_logger.h"

#include "cmsis_os2.h"

static void acquisition_task(void *argument)
{
    board_adc_block_t block;

    (void)argument;

    for (;;) {
        if (board_adc_get_block(&block, osWaitForever)) {
            (void)obs_network_submit_adc(&block);
            (void)obs_usb_logger_submit(&block);
        }
    }
}

static void sensor_task(void *argument)
{
    board_depth_data_t depth;
    board_analog_data_t analog;

    (void)argument;

    for (;;) {
        board_imu_data_t imu = board_imu_get();

        obs_state_set_imu(imu);

        if (board_depth_read(&depth)) {
            obs_state_set_depth(depth);
        }

        if (board_analog_read(&analog)) {
            obs_state_set_analog(analog);
        }

        osDelay(OBS_SENSOR_PERIOD_MS);
    }
}

static void usb_logger_task(void *argument)
{
    (void)argument;

    for (;;) {
        obs_usb_logger_service();
        osDelay(5U);
    }
}

void app_tasks_create(void)
{
    const osThreadAttr_t acquisition_attributes = {
        .name = "adc",
        .priority = osPriorityHigh,
        .stack_size = 2048U,
    };

    const osThreadAttr_t network_attributes = {
        .name = "net",
        .priority = osPriorityAboveNormal,
        .stack_size = 4096U,
    };

    const osThreadAttr_t sensor_attributes = {
        .name = "sensor",
        .priority = osPriorityNormal,
        .stack_size = 2048U,
    };

    const osThreadAttr_t usb_attributes = {
        .name = "usb",
        .priority = osPriorityNormal,
        .stack_size = 4096U,
    };

    (void)osThreadNew(acquisition_task, NULL, &acquisition_attributes);
    (void)osThreadNew(obs_network_task, NULL, &network_attributes);
    (void)osThreadNew(sensor_task, NULL, &sensor_attributes);
    (void)osThreadNew(usb_logger_task, NULL, &usb_attributes);
}
