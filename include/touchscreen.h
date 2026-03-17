#pragma once

#ifndef DESKTOPSIM
#include "gpio.h"
#else
#include <stdint.h>
#endif

struct TouchscreenRawInput
{
    int16_t x, y, z1, z2;
};

struct TouchscreenEvent
{
    enum Type
    {
        NONE,
        OFF,
        RELEASE,
        PRESS,
        ON,
    };

    Type type;
    int x, y, z;
    
    TouchscreenRawInput raw;
};

class Touchscreen
{
#ifndef DESKTOPSIM
public:
    static const unsigned NSAMPLES = 8;

    DMA_HandleTypeDef DMATx, DMARx;
    SPI_HandleTypeDef SPI;
    uint8_t txvalues[16], rxvalues[16];
    TouchscreenRawInput scanbuf[NSAMPLES], avg;
    volatile bool ready : 1;
    volatile bool in_progress : 1;
    unsigned cycle;
    void onDataReady();
#endif
public:
    static Touchscreen *instance;
    bool pressed;

    bool quit;
    void init();
    TouchscreenEvent::Type poll(TouchscreenEvent &event);
#ifndef DESKTOPSIM
    void onSystick();
#endif
};
