#include "buzzer.h"
#include <math.h>
#include <unistd.h>

#ifndef DESKTOPSIM
#include "gpio.h"
#else
#include <sys/ioctl.h>
#include <linux/kd.h>
#endif

void Buzzer::init()
{
    off();
}

#ifndef DESKTOPSIM    
void Buzzer::onImpl(int period)
{
    Pin{GPIOE, 5}.setAsAlt(3); // AF3_TIM9
    
    TIM_HandleTypeDef timer = {.Instance = TIM9};
    TIM_OC_InitTypeDef channelConfig = {};

    timer.Init.Period = period;
    timer.Init.Prescaler = ((SystemCoreClock) / timebase) - 1;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_OC_Init(&timer);
    
    channelConfig.OCMode = TIM_OCMODE_PWM1;
    channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    channelConfig.Pulse = timer.Init.Period >> 1;
    HAL_TIM_OC_ConfigChannel(&timer, &channelConfig, TIM_CHANNEL_1);
    // HAL_TIM_OC_Stop(&timer, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&timer, TIM_CHANNEL_1);
}
#endif

void Buzzer::off()
{
#ifndef DESKTOPSIM
    TIM_HandleTypeDef timer = {.Instance = TIM9};
    HAL_TIM_OC_Stop(&timer, TIM_CHANNEL_1);

    Pin(GPIOE, 5).setAsOutput().reset();
#endif
}

#ifndef DESKTOPSIM
void Buzzer::beepImpl(int period, int dur)
{
    onImpl(period);
    HAL_Delay(dur);
    off();
}
#endif

void Buzzer::click()
{
    beep(24, 2);
}
