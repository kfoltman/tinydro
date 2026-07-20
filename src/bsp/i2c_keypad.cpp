#include "i2c_keypad.h"
#include "gpio.h"

#ifdef HAS_KEYPAD

void I2C_Keypad::init()
{
    Pin{GPIOB, 8}.setAsAlt(4).setAsOpenDrain();
    Pin{GPIOB, 9}.setAsAlt(4).setAsOpenDrain();
    Pin{GPIOE, 0}.setAsInputPullup();
    Pin{GPIOE, 1}.setAsInputPullup();
    Pin{GPIOE, 2}.setAsInputPullup();
    Pin{GPIOE, 3}.setAsInputPullup();
    
    i2c.Instance = I2C1;
    i2c.Init.Timing = (12 << 28) | (15 << 20) | (15 << 16) | (60 << 8) | 60; // 12 MHz : 120 = 100 kHz
    i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&i2c);

    lastKey = -1;
    keyTimer = 0;
}

void I2C_Keypad::poll()
{
    uint8_t data[1] = { uint8_t(~(1 << row)) };
    if (HAL_I2C_Master_Transmit(&i2c, i2c_addr, data, 1, 100) == HAL_OK) {
        if (HAL_I2C_Master_Receive(&i2c, i2c_addr, data, 1, 100) == HAL_OK) {
            uint8_t port = ~data[0];
            uint32_t col = 0;
            uint32_t shift = 6 * row;
            for (int i = 0; i < 4; ++i) {
                col |= (!Pin{GPIOE, i}.read()) ? (1 << i) : 0;
            }
            col |= port & 0x30;
            keys = (keys & ~(0b111111 << shift)) | (col << shift);
            if (row == 3) { // shift
                uint32_t shiftmask = 1 << 24;
                keys = port & 0x40 ? (keys | shiftmask) : (keys &~ shiftmask);
            }
            row = (row + 1) & 3;
        }
    }
}

int I2C_Keypad::pollFull()
{
    for (int i = 0; i < 4; ++i)
        poll();

    int key = -1;
    for (int i = 0; i < 24; ++i) {
        if (keys & (1 << i)) {
            key = i;
            break;
        }
    }
    if (keyTimer > 0) {
        keyTimer--;
        return -1;
    }
    if (key == -1) {
        lastKey = -1;
        return -1;
    }
    if (key == lastKey) {
        keyTimer = 5;
        return key | 0x40;
    }
    keyTimer = 5;
    lastKey = key;
    return key;
}

#endif
