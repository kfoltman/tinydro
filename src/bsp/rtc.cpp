#include "rtc.h"
#include <string.h>

#ifdef HAS_RTC

void RealTimeClock::init()
{
    memset(&handle, 0, sizeof(handle));
    handle.Instance = RTC;
    handle.Init.AsynchPrediv = 127;
    handle.Init.SynchPrediv = 255;
    handle.Init.HourFormat = RTC_HOURFORMAT_24;
    init_error = HAL_RTC_Init(&handle);
}

RealTimeClock::Time RealTimeClock::getTime()
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    if (init_error) {
        return Time{};
    }
    if (HAL_RTC_GetTime(&handle, &time, RTC_FORMAT_BIN) == HAL_OK) {
        HAL_RTC_GetDate(&handle, &date, RTC_FORMAT_BIN);
        return Time{date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds, uint16_t(time.SubSeconds * 1000U / time.SecondFraction)};
    }
    return Time{};
}

void RealTimeClock::setDate(const RealTimeClock::Time &time)
{
    RTC_DateTypeDef date;
    date.Year = time.year;
    date.Month = time.month;
    date.Date = time.day;
    HAL_RTC_SetDate(&handle, &date, RTC_FORMAT_BIN);
}

void RealTimeClock::setTime(const RealTimeClock::Time &timeval)
{
    RTC_TimeTypeDef time;
    HAL_RTC_GetTime(&handle, &time, RTC_FORMAT_BIN);
    time.Hours = timeval.hour;
    time.Minutes = timeval.minute;
    time.Seconds = timeval.second;
    time.SubSeconds = uint32_t((timeval.subsecond * time.SecondFraction + 500U) / 1000U);
    HAL_RTC_SetTime(&handle, &time, RTC_FORMAT_BIN);
    // GetTime latches the time until GetDate is called
    RTC_DateTypeDef date;
    HAL_RTC_GetDate(&handle, &date, RTC_FORMAT_BIN);
}

#endif
