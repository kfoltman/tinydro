#pragma once

#include "eeprom.h"
#include "screen.h"
#include "droscreens.h"
#include "calc.h"
#include "touchscreen.h"
#include "encoder.h"
#ifdef HAS_WS2812B
#include "ws2812b.h"
#endif
#ifdef HAS_KEYPAD
#include "i2c_keypad.h"
#endif

class App
{
protected:
    int cur_coord{-1};
    int serial_coord{-1};
    std::string serial_value;
    uint16_t last_poll_ms;

    void editCoord(int coord);
    void goToCalc(const std::string &text, double value);

public:
    MainScreen mainScreen;
    CalcScreen calcScreen;
    CalibrationScreen calibrationScreen;
    Screen *screen;    
    EEPROM eeprom;
    Touchscreen touchscreen;
    Encoder encX{4}, encY{3}, encZ{2};
#ifdef HAS_WS2812B
    WS2812B rgbled;
#endif
#ifdef HAS_KEYPAD
    I2C_Keypad keypad;
#endif

    void init();
    void loop();
    void initThermo();
    void onEvent(const TouchscreenEvent *event);
    void onSerialChar(char ch);
    void render(Display &display);
    void updateEncoders();
};

extern App app;
