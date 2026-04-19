#include "app.h"
#include "buzzer.h"
#include "gpio.h"

int globalTime{0};

Buzzer buzzer;

void App::init()
{
    eeprom.init();
    buzzer.init();
    touchscreen.init();
    encX.init();
    encY.init();
    encZ.init();

    mainScreen.init();
    calcScreen.init();
    calcScreen.onEnter = [this](float value) -> std::string {
        mainScreen.setCoord(cur_coord, mainScreen.fromDisplay(value));
        screen = &mainScreen;
        screen->dirty();
        return std::string();
    };
    calcScreen.onEsc = [this]() {
        screen = &mainScreen;
        screen->dirty();
    };
    calibrationScreen.init();

    screen = &mainScreen;

    mainScreen.x_coord.action = [this]() { editCoord(0); };
    mainScreen.y_coord.action = [this]() { editCoord(1); };
    mainScreen.z_coord.action = [this]() { editCoord(2); };
    mainScreen.buttonFunc.action = [this]() {
        app.eeprom.write32(0, app.eeprom.read32(0) + 1);
        app.eeprom.flush();
    };
    mainScreen.buttonSetup.action = [this]() {
        screen = &calibrationScreen;
        screen->dirty();
        calibrationScreen.startCalibration();
    };
    last_poll_ms = HAL_GetTick();
}

void App::goToCalc(const std::string &text, double value)
{
    screen = &calcScreen;
    calcScreen.ask(text, value);
    screen->dirty();
}

void App::editCoord(int coord)
{
    cur_coord = coord;
    goToCalc(std::string("Enter ") + char('X' + cur_coord) + " value:", mainScreen.toDisplay(mainScreen.getCoord(cur_coord)));
}

void App::updateEncoders()
{
    int delta = encX.delta();
    mainScreen.pos.coord(0) += delta;
    mainScreen.abs.coord(0) += delta;
    delta = encY.delta();
    mainScreen.pos.coord(1) += delta;
    mainScreen.abs.coord(1) += delta;
    delta = encZ.delta();
    mainScreen.pos.coord(2) += delta;
    mainScreen.abs.coord(2) += delta;
}

void App::loop()
{
    uint16_t poll_ms = HAL_GetTick();
    if (uint16_t(poll_ms - last_poll_ms) >= 10)
    {
        
        TouchscreenEvent event;
        touchscreen.poll(event);
        onEvent(&event);
        last_poll_ms = poll_ms;
#if 0
        static int cnt = 64;
        if (cnt & 128)
            screen->scroll(-4, 2);
        else
            screen->scroll(4, -2);
        cnt++;
#endif
    }
    
    screen->loop();
    if (screen == &calibrationScreen && calibrationScreen.calibrationDone()) {
        screen = &mainScreen;
        screen->dirty();
        return;
    }
}

void App::onEvent(const TouchscreenEvent *event)
{
    screen->onEvent(event);
}

void App::render(Display &display)
{
    screen->render(display);
}

void App::onSerialChar(char ch)
{
    switch(ch) {
        case 'X': serial_coord = 0; break;
        case 'Y': serial_coord = 1; break;
        case 'Z': serial_coord = 2; break;
        case '-':
            if (serial_value.empty())
                serial_value = ch;
            break;
        case '0'...'9':
            if (serial_value.length() < 20)
                serial_value += ch;
            break;
        case '\r':
            if (serial_coord >= 0 && serial_coord <= 2 && serial_value.length()) {
                mainScreen.setCoord(serial_coord, atoi(serial_value.c_str()));
            }
            serial_coord = -1;
            serial_value.clear();
            break;
    }
}