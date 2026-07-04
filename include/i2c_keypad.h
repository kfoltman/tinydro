#pragma once

#include "hwdefs.h"

class I2C_Keypad
{
private:
    I2C_HandleTypeDef i2c{};
    static const uint32_t i2c_addr = 0x20 << 1;
public:
    uint32_t keys{0};
    uint8_t row{0};

    void init();
    void poll();
};

