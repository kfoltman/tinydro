#include "gpio.h"

class Thermocouple
{
protected:
    static void wait();
    uint8_t readSPI();
    void writeSPI(uint8_t value);
public:
    void init();
    uint8_t readReg(uint8_t reg);
    void writeReg(uint8_t reg, uint8_t value);
    uint32_t readTemp();
};

