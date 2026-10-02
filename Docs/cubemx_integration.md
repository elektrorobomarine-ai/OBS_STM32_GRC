# CubeMX / CubeIDE Integration Guide

## Required middleware
- FreeRTOS
- LwIP
- FATFS
- USB_HOST / MSC

## Recommended cache / DMA strategy for STM32H743

STM32H7 has D-Cache. DMA buffers require special attention.

Recommended options:
1. Place high-rate DMA buffers in non-cacheable D2 SRAM using MPU, or
2. Perform `SCB_InvalidateDCache_by_Addr()` / `SCB_CleanDCache_by_Addr()` correctly.

For SAI RX ADC acquisition, the preferred approach is a dedicated non-cacheable DMA region.
Do not put SAI DMA buffers in DTCM because many DMA masters cannot access DTCM.

## Main integration

After generated peripheral initialization:

```c
MX_GPIO_Init();
MX_DMA_Init();
MX_ETH_Init();
MX_SPI2_Init();
MX_SAI1_Init();
MX_USART3_UART_Init();
MX_I2C1_Init();
MX_ADC1_Init();
MX_TIM1_Init();
MX_USB_HOST_Init();

if (!app_init()) {
    Error_Handler();
}
app_tasks_create();
if (!app_start()) {
    Error_Handler();
}
```

If CubeMX creates tasks inside `MX_FREERTOS_Init()`, call `app_tasks_create()` from there.

## SAI callback hook

Add to a user code section:

```c
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
```

## USART3 receive
Use `HAL_UARTEx_ReceiveToIdle_DMA()` for WT901 and feed received bytes into the IMU parser.

## Ethernet
For static IP default:
- MCU: 192.168.3.101
- netmask: 255.255.255.0
- gateway: 192.168.3.1
- data UDP: 5005
- control TCP: 5010

These are all centralized in `App/Inc/app_config.h`.

## USB
For USB flash disk logging:
- Mode: Host
- Class: MSC
- FATFS linked to USB disk
- D-: PA11
- D+: PA12
- Use external current-limited 5 V VBUS switch

## UTC
All binary timestamps and generated filenames must use UTC.
