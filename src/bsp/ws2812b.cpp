#include "ws2812b.h"
#include "gpio.h"

#define PERIOD (150 * 2)

static const uint32_t HI = (90 * PERIOD / 125);
static const uint32_t LO = (35 * PERIOD / 125);

__attribute__((aligned(32)))
static volatile uint32_t pwm_buffer[48 + 24 * WS2812B::NUM_LEDS + 24];

WS2812B *WS2812B::instance;

extern "C" void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&WS2812B::instance->hdma_tim5_ch2);
}

extern "C" void Error_Handler(void);

void WS2812B::init()
{
    Pin{GPIOA, 1}.setAsAlt(2); // TIM5_CH2
    TIM_OC_InitTypeDef channelConfig = {};

    timer.Init.Period = PERIOD;
    timer.Init.Prescaler = 0;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;    
    HAL_TIM_PWM_Init(&timer);

    channelConfig.OCMode = TIM_OCMODE_PWM1;
    channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
    channelConfig.Pulse = timer.Init.Period; // 2 * timer.Init.Period / 3;
    HAL_TIM_PWM_ConfigChannel(&timer, &channelConfig, TIM_CHANNEL_2);

    hdma_tim5_ch2.Instance = DMA1_Stream0;
    hdma_tim5_ch2.Init.Request = DMA_REQUEST_TIM5_UP;
    hdma_tim5_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim5_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim5_ch2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim5_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim5_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim5_ch2.Init.Mode = DMA_NORMAL;
    hdma_tim5_ch2.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_tim5_ch2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_tim5_ch2.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    hdma_tim5_ch2.Init.MemBurst = DMA_MBURST_INC4;
    hdma_tim5_ch2.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&hdma_tim5_ch2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&timer,hdma[TIM_DMA_ID_CC2],hdma_tim5_ch2);
    __HAL_TIM_ENABLE_DMA(&timer, TIM_DMA_UPDATE);
    __HAL_TIM_DISABLE_DMA(&timer, TIM_DMA_CC2);
        
    WS2812B::instance = this;

    HAL_Delay(10);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void WS2812B::update()
{
    HAL_TIM_PWM_Stop_DMA(&timer, TIM_CHANNEL_2);
    for (int led = 0; led < NUM_LEDS; ++led) {
        for (int i = 0; i < 24; ++i) {
            pwm_buffer[48 + led * 24 + i] = (colours[led] & (1 << (23 - i))) ? HI : LO;
        }
    }
    SCB_CleanDCache_by_Addr((uint32_t *)pwm_buffer, sizeof(pwm_buffer));
    HAL_TIM_PWM_Start_DMA(&timer, TIM_CHANNEL_2, (const uint32_t *)pwm_buffer, sizeof(pwm_buffer) / 4);
}

