#pragma once

#include "eeprom.h"
#include "screen.h"
#include "calc.h"
#include "touchscreen.h"
#include "encoder.h"

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

    void init();
    void loop();
    void initThermo();
    void onEvent(const TouchscreenEvent *event);
    void onSerialChar(char ch);
    void render(Display &display);
    void updateEncoders();
};

extern App app;
