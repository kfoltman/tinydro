#pragma once

#ifdef DESKTOPSIM

#define EEPROM_IS_EMULATED

#else

#ifdef BOARD_V4
#define MCU_IS_STM32H7
#else
#define MCU_IS_STM32F4
#ifdef BOARD_V2
#define MCU_IS_STM32F427
#else
#define MCU_IS_STM32F407
#endif
#endif

#ifdef MCU_IS_STM32F4
#include <stm32f4xx_hal.h>
#endif
#ifdef MCU_IS_STM32H7
#include <stm32h7xx_hal.h>
#endif

#undef UNUSED
#define UNUSED(x) do { uint32_t __attribute__((unused)) tmptmp = (uint32_t)(x); } while(0)

#ifdef MCU_IS_STM32F407
#define __HAL_RCC_FMC_CLK_ENABLE __HAL_RCC_FSMC_CLK_ENABLE
#endif

// Buzzer configuration
#define BUZZER_PIN Pin{GPIOE, 5}
#ifdef MCU_IS_STM32H7
#define BUZZER_TIMER TIM15
#define BUZZER_PIN_AF 4
#define HAL_BUZZER_CLK_ENABLE __HAL_RCC_TIM15_CLK_ENABLE
#else
#define BUZZER_TIMER TIM9
#define BUZZER_PIN_AF 3
#define HAL_BUZZER_CLK_ENABLE __HAL_RCC_TIM9_CLK_ENABLE
#endif

// Touchscreen configuration
#ifdef MCU_IS_STM32H7
#define TOUCHSCREEN_DMA_REQUEST_TX DMA_REQUEST_SPI2_TX
#define TOUCHSCREEN_DMA_REQUEST_RX DMA_REQUEST_SPI2_RX
#else
#define TOUCHSCREEN_DMA_CHANNEL DMA_CHANNEL_0
#endif

// TFT LCD
#define LCD_PWM_TIMER TIM12
#define LCD_PWM_PIN Pin{GPIOB, 14}
#ifdef MCU_IS_STM32H7
#define LCD_PWM_PIN_AF 2
#else
#define LCD_PWM_PIN_AF 9
#endif
#if !defined(BOARD_V2) && !defined(BOARD_V4)
#define LCD_RESET_HACK
#else
#define LCD_IS_IPS
#endif

// EEPROM
#ifdef BOARD_V4
#define EEPROM_IS_EXTERNAL
#else
#define EEPROM_IS_EMULATED
#endif

// WS2812B LEDs
#ifdef BOARD_V4
#define HAS_WS2812B
#endif

// Keypad
#ifdef BOARD_V4
#define HAS_KEYPAD
#endif

// RTC
#ifdef BOARD_V4
#define HAS_RTC
#endif

#endif
