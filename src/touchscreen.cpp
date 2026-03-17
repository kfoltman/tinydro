#include "touchscreen.h"
#include <algorithm>

Touchscreen *Touchscreen::instance = nullptr;

#ifdef DESKTOPSIM

#include <SDL/SDL.h>

void Touchscreen::init()
{
    instance = this;
    quit = false;
}

TouchscreenEvent::Type Touchscreen::poll(TouchscreenEvent &event)
{
    SDL_Event sdlEvent;
    event.type = TouchscreenEvent::NONE;
    while(SDL_PollEvent(&sdlEvent)) {
        switch(sdlEvent.type)
        {
            case SDL_MOUSEBUTTONDOWN:
                if (sdlEvent.button.button == 1) {
                    pressed = true;
                    event.type = TouchscreenEvent::PRESS;
                    event.x = sdlEvent.button.x;
                    event.y = sdlEvent.button.y;
                    event.z = 10000;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (sdlEvent.button.button == 1) {
                    pressed = false;
                    event.type = TouchscreenEvent::RELEASE;
                    event.x = sdlEvent.button.x;
                    event.y = sdlEvent.button.y;
                    event.z = 0;
                }
                break;
            case SDL_MOUSEMOTION:
                event.type = pressed ? TouchscreenEvent::ON : TouchscreenEvent::OFF;
                event.x = sdlEvent.motion.x;
                event.y = sdlEvent.motion.y;
                event.z = pressed ? 10000 : 0;
                break;
            case SDL_QUIT:
                quit = true;
                break;
        }
    }
    return event.type;
}

#else

#include <cstdlib>

#define TOUCH_DIN Pin{GPIOC, 3}
#define TOUCH_DOUT Pin{GPIOC, 2}
#define TOUCH_SCK Pin{GPIOB, 13}
#define TOUCH_CSN Pin{GPIOA, 2}
#define TOUCH_BUSY Pin{GPIOA, 3}
#define TOUCH_PENIRQ Pin{GPIOC, 1}

void Touchscreen::init()
{
    instance = this;
    TOUCH_DIN.setAsAlt(5);
    TOUCH_DOUT.setAsAlt(5);
    TOUCH_SCK.setAsAlt(5);
    TOUCH_CSN.setAsOutput();
    TOUCH_BUSY.setAsInput();
    TOUCH_PENIRQ.setAsInputPullup();
    
    TOUCH_CSN.set();
    
    SPI.Instance = SPI2;
    SPI.Init.Mode = SPI_MODE_MASTER;
    SPI.Init.Direction = SPI_DIRECTION_2LINES;
    SPI.Init.DataSize = SPI_DATASIZE_8BIT;
    SPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    SPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    SPI.Init.NSS = SPI_NSS_SOFT;
    SPI.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    SPI.Init.FirstBit = SPI_FIRSTBIT_MSB;
    SPI.Init.TIMode = SPI_TIMODE_DISABLE;
    SPI.Init.CRCCalculation = false;
    SPI.Init.CRCPolynomial = 0;
    HAL_SPI_Init(&SPI);
    
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    DMARx.Instance = DMA1_Stream3;
    DMARx.Init.Channel = DMA_CHANNEL_0;
    DMARx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    DMARx.Init.PeriphInc = DMA_PINC_DISABLE;
    DMARx.Init.MemInc = DMA_MINC_ENABLE;
    DMARx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    DMARx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    DMARx.Init.Mode = DMA_NORMAL;
    DMARx.Init.Priority = DMA_PRIORITY_LOW;
    DMARx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    DMARx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    DMARx.Init.MemBurst = DMA_MBURST_SINGLE;
    DMARx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&DMARx);
    
    DMATx.Instance = DMA1_Stream4;
    DMATx.Init.Channel = DMA_CHANNEL_0;
    DMATx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    DMATx.Init.PeriphInc = DMA_PINC_DISABLE;
    DMATx.Init.MemInc = DMA_MINC_ENABLE;
    DMATx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    DMATx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    DMATx.Init.Mode = DMA_NORMAL;
    DMATx.Init.Priority = DMA_PRIORITY_LOW;
    DMATx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    DMATx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    DMATx.Init.MemBurst = DMA_MBURST_SINGLE;
    DMATx.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&DMATx);
    
    __HAL_LINKDMA(&SPI, hdmarx, DMARx);
    __HAL_LINKDMA(&SPI, hdmatx, DMATx);
    
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 8, 1);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    
    int mode = 0;
    int ser = 0;
    int pd = 3;
    auto ctl = [mode, ser, pd](int addr) -> uint8_t {
        return 0x80 | (addr << 4) | ((mode != 0) << 3) | (ser << 2) | pd;
    };
    for(uint8_t &value: txvalues)
        value = 0;
    for(uint8_t &value: rxvalues)
        value = 0;
    txvalues[0] = ctl(1);
    txvalues[4] = ctl(5);
    txvalues[8] = ctl(3);
    txvalues[12] = ctl(4);
    in_progress = false;
    cycle = 0;
    pressed = false;
    quit = false;
    ready = true;
}

#if 0
static inline int CycleXPTBit()
{
    for (volatile int i = 0; i < 40; ++i)
        ;
    TOUCH_SCK.set();
    int value = TOUCH_DOUT.read();
    for (volatile int i = 0; i < 40; ++i)
        ;
    TOUCH_SCK.reset();
    return value;
}

int ShiftXPT_sw(int addr, int mode, int ser, int pd)
{
    TOUCH_CSN.reset();
    TOUCH_DIN.set(); // Start bit
    CycleXPTBit();
    TOUCH_DIN.write(addr & 4); // A2
    CycleXPTBit();
    TOUCH_DIN.write(addr & 2); // A1
    CycleXPTBit();
    TOUCH_DIN.write(addr & 1); // A0
    CycleXPTBit();
    TOUCH_DIN.write(mode != 0); // mode
    CycleXPTBit();
    TOUCH_DIN.write(ser); // ser/dfr
    CycleXPTBit();
    TOUCH_DIN.write(pd & 2); // pd1
    CycleXPTBit();
    TOUCH_DIN.write(pd & 1); // pd0
    CycleXPTBit();
    TOUCH_DIN.write(0); // avoid DIN being interpreted as a start bit
    CycleXPTBit();
    while(TOUCH_BUSY.read())
        ;
    int result = 0;
    for (int i = 0; i < 16; ++i) {
        result = result * 2 + CycleXPTBit();
    }
    
    TOUCH_CSN.set();
    for (volatile int i = 0; i < 240; ++i)
        ;
    return result;
}
#endif

void Touchscreen::onSystick()
{
    if (ready && !in_progress) {
        in_progress = true;
        TOUCH_CSN.reset();
        HAL_SPI_TransmitReceive_DMA(&SPI, txvalues, rxvalues, sizeof(txvalues));
    }
}

void Touchscreen::onDataReady()
{
    TOUCH_CSN.set();
    int x = (rxvalues[1] * 256 + rxvalues[2]) >> 3;
    int y = (rxvalues[5] * 256 + rxvalues[6]) >> 3;
    int z1 = (rxvalues[9] * 256 + rxvalues[10]) >> 3;
    int z2 = (rxvalues[13] * 256 + rxvalues[14]) >> 3;
    scanbuf[cycle].x = x;
    scanbuf[cycle].y = y;
    scanbuf[cycle].z1 = z1;
    scanbuf[cycle].z2 = z2;
    cycle += 1;
    if (cycle >= NSAMPLES)
        cycle = 0;
    else {
        in_progress = false;
        return;
    }
    TouchscreenRawInput sum{};
    TouchscreenRawInput max{};
    TouchscreenRawInput min{32767, 32767, 32767, 32767};
    for (unsigned i = 0; i < NSAMPLES; ++i) {
        TouchscreenRawInput raw = scanbuf[i];
        sum.x += raw.x;
        sum.y += raw.y;
        sum.z1 += raw.z1;
        sum.z2 += raw.z2;
        max.x = std::max(max.x, raw.x);
        max.y = std::max(max.y, raw.y);
        max.z1 = std::max(max.z1, raw.z1);
        max.z2 = std::max(max.z2, raw.z2);
        min.x = std::min(min.x, raw.x);
        min.y = std::min(min.y, raw.y);
        min.z1 = std::min(min.z1, raw.z1);
        min.z2 = std::min(min.z2, raw.z2);
    }
    unsigned dev = (max.x - min.x) + (max.y - min.y) + (max.z1 - min.z1) + (max.z2 - min.z2);
    if (dev < 8 * NSAMPLES) {
        sum.x /= NSAMPLES;
        sum.y /= NSAMPLES;
        sum.z1 /= NSAMPLES;
        sum.z2 /= NSAMPLES;
        avg = sum;
    }
    in_progress = false;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    Touchscreen::instance->onDataReady();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
}

extern "C" void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&Touchscreen::instance->DMARx);
}

extern "C" void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&Touchscreen::instance->DMATx);
}

TouchscreenEvent::Type Touchscreen::poll(TouchscreenEvent &event)
{
    int x = avg.x;
    int y = avg.y;
    int z1 = avg.z1;
    int z2 = avg.z2;
    int z = 0;
    event.raw.x = x;
    event.raw.y = y;
    event.raw.z1 = z1;
    event.raw.z2 = z2;
    if (z1) {
        z = x * z2 / z1 - x;
    }
    int calLX = 224, calRX = 3820;
    int calBY = 280, calTY = 3750;
    
    x = (x - calLX) * 480 / (calRX - calLX);
    if (x < 0) x = 0;
    if (x > 479) x = 479;
    y = (y - calTY) * 320 / (calBY - calTY);
    if (y < 0) y = 0;
    if (y > 319) y = 319;
    event.x = x;
    event.y = y;
    event.z = y;

    bool nowPressed = z && z < 200000;
    if (nowPressed == pressed)
        event.type = pressed ? TouchscreenEvent::ON : TouchscreenEvent::OFF;
    else
        event.type = nowPressed ? TouchscreenEvent::PRESS : TouchscreenEvent::RELEASE;
    pressed = nowPressed;
    return event.type;
}

#endif
