#ifndef DESKTOPSIM
#endif
#include <assert.h>
#include "gpio.h"
#include "encoder.h"

#ifndef DESKTOPSIM
TIM_TypeDef * TimerFromId(int no)
{
    switch(no)
    {
    case 2: return TIM2;
    case 3: return TIM3;
    case 4: return TIM4;
    default:
        assert(false);
    }
    return 0;
}
#endif

Encoder::Encoder(int _id)
: id(_id)
{
}

void Encoder::init()
{
#ifndef DESKTOPSIM
    TIM_HandleTypeDef handle;
    handle.Instance = TimerFromId(id);
    handle.Init.Period = 65535;
    handle.Init.Prescaler = 0;
    handle.Init.ClockDivision = 0;
    handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    handle.Init.RepetitionCounter = 0;
    handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    TIM_Encoder_InitTypeDef init = {
        .EncoderMode = TIM_ENCODERMODE_TI12,
        .IC1Polarity = TIM_ICPOLARITY_RISING,
        .IC1Selection = TIM_ICSELECTION_DIRECTTI,
        .IC1Prescaler = TIM_ICPSC_DIV1,
        .IC1Filter = 0,
        .IC2Polarity = TIM_ICPOLARITY_RISING,
        .IC2Selection = TIM_ICSELECTION_DIRECTTI,
        .IC2Prescaler = TIM_ICPSC_DIV1,
        .IC2Filter = 0,
    };
    HAL_TIM_Encoder_Init(&handle, &init);
    HAL_TIM_Encoder_Start(&handle, TIM_CHANNEL_ALL);
    switch(id) {
    case 4:
        Pin(GPIOB, 6).setAsAlt(2);
        Pin(GPIOB, 7).setAsAlt(2);
        break;
    case 3:
        Pin(GPIOB, 4).setAsAlt(2);
        Pin(GPIOB, 5).setAsAlt(2);
        break;
    case 2:
        Pin(GPIOB, 3).setAsAlt(1);
        Pin(GPIOA, 15).setAsAlt(1);
        break;
    }
    last = get();
#endif
}

int Encoder::get()
{
#ifndef DESKTOPSIM
    TIM_HandleTypeDef handle = { .Instance = TimerFromId(id) };
    return __HAL_TIM_GET_COUNTER(&handle);
#else
    return 0;
#endif
}

int Encoder::delta()
{
#ifndef DESKTOPSIM
    int16_t cur = get();
    int16_t res = cur - last;
    last = cur;
    return res;
#else
    return 0;
#endif
}

