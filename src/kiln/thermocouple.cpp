#include "thermocouple.h"

Pin userPins[8] = {
    Pin{GPIOD, 13},
    Pin{GPIOC, 6},
    Pin{GPIOC, 7},
    Pin{GPIOC, 8},
    
    Pin{GPIOC, 9},
    Pin{GPIOA, 8},
    Pin{GPIOA, 9},
    Pin{GPIOA, 10},
};

Pin Thermo_CS = userPins[0];
Pin Thermo_SCK = userPins[1];
Pin Thermo_SDO = userPins[2];
Pin Thermo_SDI = userPins[3];
Pin Thermo_nFAULT = userPins[4];
Pin Thermo_nDRDY = userPins[5];
Pin Thermo_MOSFET1 = userPins[6];
Pin Thermo_MOSFET2 = userPins[7];

void Thermocouple::init()
{
    Thermo_CS.setAsOutput();
    Thermo_SCK.setAsOutput();
    Thermo_SDO.setAsInput();
    Thermo_SDI.setAsOutput();
    Thermo_nFAULT.setAsInput();
    Thermo_nDRDY.setAsInput();

    Thermo_SCK.set();
    Thermo_CS.set();
    
    writeReg(0x80, 0x80 | 0x10 | 0x02 | 0x01);
}

void Thermocouple::wait()
{
    for(volatile int i = 0; i < 100; ++i)
        ;
}

uint8_t Thermocouple::readSPI()
{
    uint8_t value = 0;
    Thermo_SCK.set();
    for (int i = 0; i < 8; ++i) {
        Thermo_SDI.write((value >> (7 - i)) & 1);
        Thermo_SCK.reset();
        wait();
        Thermo_SCK.set();
        value = (value << 1) | Thermo_SDO.read();
        wait();
    }
    return value;
}

void Thermocouple::writeSPI(uint8_t value)
{
    Thermo_SCK.set();
    for (int i = 0; i < 8; ++i) {
        Thermo_SDI.write((value >> (7 - i)) & 1);
        Thermo_SCK.reset();
        wait();
        Thermo_SCK.set();
        wait();
        
    }
}

void Thermocouple::writeReg(uint8_t reg, uint8_t value)
{
    Thermo_CS.reset();
    writeSPI(reg);
    writeSPI(value);
    Thermo_CS.set();
}

uint8_t Thermocouple::readReg(uint8_t reg)
{
    Thermo_CS.reset();
    writeSPI(reg);
    uint8_t value = readSPI();
    Thermo_CS.set();
    return value;
}

uint32_t Thermocouple::readTemp()
{
    Thermo_CS.reset();
    writeSPI(0x0C);
    uint8_t value1 = readSPI();
    uint8_t value2 = readSPI();
    uint8_t value3 = readSPI();
    Thermo_CS.set();
    return value1 * 65536 + value2 * 256 + value3;
}

