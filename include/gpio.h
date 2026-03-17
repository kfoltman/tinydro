#pragma once

#ifdef DESKTOPSIM
#include <unistd.h>
#include <time.h>

static inline void HAL_Delay(unsigned int delay)
{
    usleep(delay * 1000ULL);
}

static inline int HAL_GetTick()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#else

#include <stm32f4xx_hal.h>

struct Pin
{
protected:
    template<unsigned bits>inline void mod(volatile uint32_t &reg, uint32_t value) const
    {
        uint32_t mask = (1 << bits) - 1;
        uint32_t shift = (bits >= 4) ? ((pin * bits) & 31) : pin * bits;
        reg = (reg & ~(mask << shift)) | (value << shift);
    }
public:
    GPIO_TypeDef *GPIO = nullptr;
    int pin = -1;
    
    Pin() = default;
    Pin(GPIO_TypeDef *_GPIO, int _pin) : GPIO{_GPIO}, pin{_pin} {}
    const Pin &setAsOutput() const { mod<2>(GPIO->MODER, 1); return *this; }
    const Pin &setAsInput() const { mod<2>(GPIO->MODER, 0); mod<2>(GPIO->PUPDR, 0); return *this; }
    const Pin &setAsInputPullup() const { mod<2>(GPIO->MODER, 0); mod<2>(GPIO->PUPDR, 1); return *this; }
    const Pin &setAsAlt(int numFunc) const {
        mod<2>(GPIO->MODER, 2);
        mod<4>(GPIO->AFR[pin >> 3], numFunc);
        return *this;
    }
    const Pin &setSpeed(uint8_t speed) const { mod<2>(GPIO->OSPEEDR, speed); return *this; }
    void set() const { GPIO->BSRR = 1 << pin; }
    void reset() const { GPIO->BSRR = 0x10000 << pin; }
    void write(bool value) const { GPIO->BSRR = 1 << (pin | (value ? 0 : 16)); }
    bool read() const { return (GPIO->IDR >> pin) & 1; }
};
#endif
