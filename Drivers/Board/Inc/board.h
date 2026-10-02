#pragma once

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SPI_HandleTypeDef hspi2;
extern SAI_HandleTypeDef hsai_BlockA1;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;
extern RTC_HandleTypeDef hrtc;

#define ADC_CS_GPIO_Port        GPIOB
#define ADC_CS_Pin              GPIO_PIN_4

#define ADC_RESET_GPIO_Port     GPIOD
#define ADC_RESET_Pin           GPIO_PIN_2

#define ADC_SYNC_IN_GPIO_Port   GPIOD
#define ADC_SYNC_IN_Pin         GPIO_PIN_3

#define ADC_SYNC_OUT_GPIO_Port  GPIOD
#define ADC_SYNC_OUT_Pin        GPIO_PIN_4

#define RELAY1_GPIO_Port        GPIOC
#define RELAY1_Pin              GPIO_PIN_6

#define RELAY2_GPIO_Port        GPIOC
#define RELAY2_Pin              GPIO_PIN_7

#ifdef __cplusplus
}
#endif
