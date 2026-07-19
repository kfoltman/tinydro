#pragma once

#include "hwdefs.h"

extern "C" void DMA1_Stream0_IRQHandler(void);

class WS2812B
{
private:
    DMA_HandleTypeDef hdma_tim5_ch2;
    TIM_HandleTypeDef timer = {.Instance = TIM5};
    
    friend void DMA1_Stream0_IRQHandler(void);

public:
    static const int NUM_LEDS = 4;
    uint32_t colours[NUM_LEDS];
    static WS2812B *instance;
public:
    WS2812B() : hdma_tim5_ch2{}, colours{} {
    }
    static constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return g * 65536U + r * 256U + b; }
    void init();
    void update();
};