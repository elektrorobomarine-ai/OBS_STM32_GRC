/*
 * Example integration snippet.
 *
 * Copy the includes and callbacks into CubeMX USER CODE sections.
 * Do not paste executable statements at file scope.
 */

#include "app.h"
#include "app_tasks.h"
#include "board_adc.h"
#include "board_imu.h"

/*
 * Call this after:
 *   - all MX_* peripheral initialization functions
 *   - osKernelInitialize()
 *
 * Then start the RTOS kernel in the normal CubeMX flow.
 */
static void OBS_ApplicationInit(void)
{
    if (!app_init()) {
        Error_Handler();
    }

    app_tasks_create();

    if (!app_start()) {
        Error_Handler();
    }
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_BlockA1) {
        board_adc_on_dma_half_complete();
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_BlockA1) {
        board_adc_on_dma_complete();
    }
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    if (hsai == &hsai_BlockA1) {
        board_adc_on_sai_error();
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart == &huart3) {
        board_imu_on_rx_event(size);
    }
}
