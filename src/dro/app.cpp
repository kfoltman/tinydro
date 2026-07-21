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
#ifdef HAS_WS2812B
    rgbled.init();
#endif
#ifdef HAS_KEYPAD
    keypad.init();
#endif
#ifdef HAS_RTC
    rtc.init();
#endif

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
        mainScreen.menu.flags ^= WF_HIDDEN;
        mainScreen.menu.dirty();
        screen->dirty();
        // app.eeprom.write32(0, app.eeprom.read32(0) + 1);
        // app.eeprom.flush();
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
    const uint16_t frame_time_setting = 20;
    uint16_t poll_ms = HAL_GetTick();
    uint16_t frame_time = poll_ms - last_poll_ms;
    if (frame_time < frame_time_setting)
    {
        HAL_Delay(frame_time_setting - frame_time);
        return;
    }
    last_poll_ms = poll_ms;

#ifdef HAS_WS2812B
    constexpr uint32_t red = WS2812B::rgb(255, 0, 0);
    constexpr uint32_t black = WS2812B::rgb(0, 0, 0);
    rgbled.colours[0] = cur_coord == 0 && screen == &calcScreen ? red : black;
    rgbled.colours[1] = cur_coord == 1 && screen == &calcScreen ? red : black;
    rgbled.colours[2] = cur_coord == 2 && screen == &calcScreen ? red : black;
    rgbled.colours[3] = black;
    rgbled.update();
#endif
#ifdef HAS_KEYPAD
    int key = keypad.pollFull();
    if (key != -1) {
        onKey(key);
    }
#endif
#ifdef HAS_RTC
    RealTimeClock::Time t = rtc.getTime();
    char buf[64];
    sprintf(buf, "%02d:%02d:%02d.%03d", (int)t.hour, (int)t.minute, (int)t.second, (int)t.subsecond);
    mainScreen.debug_label.setText(buf);
#endif
    TouchscreenEvent event;
    touchscreen.poll(event);
    onEvent(&event);

    screen->onTick();
    screen->loop();
    if (screen == &calibrationScreen && calibrationScreen.calibrationDone()) {
        screen = &mainScreen;
        screen->dirty();
        return;
    }
}

void App::onKey(int key)
{
#ifdef HAS_KEYPAD
    switch(key) {
        case 18: editCoord(0); break;
        case 12: editCoord(1); break;
        case 6: editCoord(2); break;
        case 0: screen = &mainScreen; screen->dirty(); break;

#ifdef HAS_RTC
        case 19: { RealTimeClock::Time t = rtc.getTime(); t.hour = (t.hour + 1) % 24; rtc.setTime(t); break; }
        case 13: { RealTimeClock::Time t = rtc.getTime(); t.minute = (t.minute + 1) % 60; rtc.setTime(t); break; }
        case 7: { RealTimeClock::Time t = rtc.getTime(); t.second = (t.second + 1) % 60; rtc.setTime(t); break; }
#endif
        default:
            break;
    }
#endif
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