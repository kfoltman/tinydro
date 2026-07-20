#pragma once

#include "hwdefs.h"

#ifdef HAS_RTC

class RealTimeClock
{
private:
#ifdef MCU_IS_STM32H7
    RTC_HandleTypeDef handle;
#endif
public:
    struct Time {
        uint8_t year{0}, month{0}, day{0};
        uint8_t hour{0}, minute{0}, second{0};
        uint16_t subsecond{0};

        Time() = default;
        Time(uint8_t _y, uint8_t _mo, uint8_t _d, uint8_t _h, uint8_t _mi, uint8_t _s, uint16_t _ss) 
        : year(_y), month(_mo), day(_d)
        , hour{_h}, minute{_mi}, second{_s}, subsecond{_ss} {}
    };
    void init();

    Time getTime();
    void setDate(const Time &time);
    void setTime(const Time &time);
};

#endif
