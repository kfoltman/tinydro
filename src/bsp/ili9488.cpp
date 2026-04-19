#include "ili9488.h"

#include "gpio.h"

#include <algorithm>
#include <initializer_list>

static void fsmcPinRange(GPIO_TypeDef *gpio, int from, int to)
{
    for (int i = from; i <= to; ++i)
        Pin{gpio, i}.setAsAlt(12).setSpeed(3);
}

static inline void LCD_CMD(uint8_t cmd, const std::initializer_list<uint8_t> &data)
{
    *lcd_ctl = cmd;
    for (uint8_t val: data)
        *lcd_data = val;
}

static inline void LCD_CMD(uint8_t cmd)
{
    *lcd_ctl = cmd;
}

static inline void LCD_DATA(uint16_t data)
{
    *lcd_data = data;
}

static inline uint16_t LCD_DATA_GET()
{
    return *lcd_data;
}

void ILI9488LCD::init()
{
    HAL_Delay(1);
    
    uint32_t bcr =  (1 << 14) |  // EXTMOD (diffferent timings for R vs W)
                    (1 << 12) |  // write enable
                    (1 << 7) | // reserved
                    (1 << 4) | // 16 bit bus width
                    (0 << 2) | // SRAM/ROM
                    1; 
    
    uint32_t btr = (0 << 20) | (3 << 16) | (3 << 8) | (3 << 4) | (3 << 0);
    uint32_t bwtr = (0 << 20) | (2 << 16) | (2 << 8) | (2 << 4) | (2 << 0);

    btr = (1 << 16) | (1 << 8) | (1 << 4) | (1 << 0);
    bwtr = (0 << 20) | (1 << 16) | (1 << 8) | (1 << 4) | (2 << 0);

    
    *(uint32_t *)0xA0000000 = bcr;
    *(uint32_t *)0xA0000004 = btr;
    *(uint32_t *)0xA0000104 = bwtr;
    HAL_Delay(10);
    
    // Set alternate function and speed
    fsmcPinRange(GPIOD, 0, 1);
    fsmcPinRange(GPIOD, 4, 5);
    fsmcPinRange(GPIOD, 7, 11);
    fsmcPinRange(GPIOD, 14, 15);
    fsmcPinRange(GPIOE, 7, 15);

#ifdef BOARD_V2
    // Dedicated reset pin
    Pin{GPIOA, 7}.setAsOutput();
    Pin{GPIOA, 7}.reset();
    HAL_Delay(30);
    Pin{GPIOA, 7}.set();
    HAL_Delay(30);
    int BRIGHTNESS = 99;
#else
    // SPI pins (PA6 acting as reset)
    Pin{GPIOA, 5}.setAsOutput();
    Pin{GPIOA, 6}.setAsOutput();
    Pin{GPIOA, 5}.reset();
    Pin{GPIOA, 6}.reset();
    HAL_Delay(2);
    Pin{GPIOA, 5}.set();
    Pin{GPIOA, 6}.set();
    HAL_Delay(130);
    int BRIGHTNESS = 50;
#endif
    
    // PWM
    Pin{GPIOB, 14}.setAsAlt(9);
    
    TIM_HandleTypeDef timer = {.Instance = TIM12};
    TIM_OC_InitTypeDef channelConfig = {};

    int timebase = 40000;
    timer.Init.Period = 100;
    timer.Init.Prescaler = ((SystemCoreClock) / timebase) - 1;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_OC_Init(&timer);

    channelConfig.OCMode = TIM_OCMODE_PWM1;
    channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    channelConfig.Pulse = BRIGHTNESS;
    HAL_TIM_OC_ConfigChannel(&timer, &channelConfig, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&timer, TIM_CHANNEL_1);
    LCD_CMD(0x11);  // Sleep OUT

    HAL_Delay(10);

    LCD_CMD(0x36, {0x28}); // memory access control
    LCD_CMD(0x3A, {0x55});  // Interface pixel format: 16-bit
    LCD_CMD(0XB0, {0x00});  // Interface Mode Control
    LCD_CMD(0xB4, {0x02}); // inversion control
    LCD_CMD(0xE9, {0x00});

    HAL_Delay(10);

    LCD_CMD(0x34);  // Tearing OFF
    LCD_CMD(0x29);  // Display ON
    LCD_CMD(0x38);  // Idle OFF
    LCD_CMD(0x13);  // Normal mode ON
#ifdef BOARD_V2
    LCD_CMD(0x21);  // Display inversion ON
#endif
}

volatile uint16_t *ILI9488LCD::startPixels(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
    LCD_CMD(0x2A, {uint8_t(sx>>8), uint8_t(sx&0xFF), uint8_t(ex>>8), uint8_t(ex&0xFF)});
    LCD_CMD(0x2B, {uint8_t(sy>>8), uint8_t(sy&0xFF), uint8_t(ey>>8), uint8_t(ey&0xFF)});
    LCD_CMD(0x2C);
    return lcd_data;
}
