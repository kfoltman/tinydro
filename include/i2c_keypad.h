#pragma once

#include "hwdefs.h"

#ifdef HAS_KEYPAD

enum KeypadButtons {
    KP_X_7 = 18,
    KP_8 = 19,
    KP_9 = 20,
    KP_DIV = 21,
    KP_POPEN = 22,
    KP_ESC = 23,

    KP_Y_4 = 12,
    KP_5 = 13,
    KP_6 = 14,
    KP_MUL = 15,
    KP_PCLOSE = 16,
    KP_BACKSPACE = 17,

    KP_Z_1 = 6,
    KP_2 = 7,
    KP_3 = 8,
    KP_MINUS = 9,
    KP_ROOT = 10,
    KP_FN = 11,

    KP_PLUSMINUS = 0,
    KP_0 = 1,
    KP_DOT = 2,
    KP_PLUS = 3,
    KP_POWER = 4,
    KP_ENTER = 5,
};

class I2C_Keypad
{
private:
    I2C_HandleTypeDef i2c{};
    static const uint32_t i2c_addr = 0x20 << 1;
public:
    uint32_t keys{0};
    uint8_t row{0};
    int8_t lastKey{-1};
    int8_t keyTimer{0};

    void init();
    void poll();
    int pollFull();
};

#endif
