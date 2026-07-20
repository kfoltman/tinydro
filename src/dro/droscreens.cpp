#include "buzzer.h"
#include "screen.h"
#include "touchscreen.h"
#include "droscreens.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

const char *TestMenu::item(int index) const {
    switch(index) {
    case 0: return "Bolt circle";
    case 1: return "Line/grid";
    case 2: return "Foo";
    case 3: return "Bar";
    case 4: return "Baz";
    default: return nullptr;
    }
}

int TestMenu::itemCount() const {
    return 5;
}

void MainScreen::init()
{
#if 0
    pos_x = LINE_WIDTH / 4;
    width = LINE_WIDTH / 2;
    pos_y = SCREEN_HEIGHT / 4;
    height = SCREEN_HEIGHT / 2;
#endif
    flags |= WF_BORDER;
    add(&x_label);
    add(&x_coord);
    add(&x_zero);
    
    add(&y_label);
    add(&y_coord);
    add(&y_zero);
    
    add(&z_label);
    add(&z_coord);
    add(&z_zero);
    
    //x_coord.z_index = 2;
    debug_label.z_index = -1;
    add(&debug_label);
    add(&menu);
    menu.flags |= WF_HIDDEN;

    add(&buttonInc);
    add(&buttonInch);
    add(&buttonMem);
    add(&buttonFunc);
    add(&buttonSetup);
    // add(&debug_label);

    x_coord.setFont(&font_lato);
    y_coord.setFont(&font_lato);
    z_coord.setFont(&font_lato);
    
    x_zero.action = [this]() { setCoord(0, 0); };
    y_zero.action = [this]() { setCoord(1, 0); };
    z_zero.action = [this]() { setCoord(2, 0); };
}

void MainScreen::loop()
{
    formatCoords();
}

void MainScreen::formatCoord(Label &display, int coordValue)
{
    int sf = 0;
    if (buttonInch.checked()) {
        sf = 10000;
    } else {
        sf = 1000;
    }
    double value = toDisplay(coordValue);
    if (value >= 100000) {
        sf /= 10;
    }
    char dest[32] = "Junk";
    switch(sf) {
        case 10000: snprintf(dest, sizeof(dest), "%0.4f", value); break;
        case 1000: snprintf(dest, sizeof(dest), "%0.3f", value); break;
        case 100: snprintf(dest, sizeof(dest), "%0.2f", value); break;
        case 10: snprintf(dest, sizeof(dest), "%0.1f", value); break;
        case 1: snprintf(dest, sizeof(dest), "%0.0f", value); break;
    }
    display.setText(dest);
}

void MainScreen::formatCoords()
{
    formatCoord(x_coord, getCoord(0));
    formatCoord(y_coord, getCoord(1));
    formatCoord(z_coord, getCoord(2));
}

int MainScreen::fromDisplay(double value) const
{
    if (buttonInch.checked())
        return round(value * 25400);
    else
        return round(value * 1000);
}

double MainScreen::toDisplay(int value) const
{
    if (buttonInch.checked())
        return value / 25400.0;
    else
        return value / 1000.0;
}


double MainScreen::getCoord(int coord) const
{
    if (buttonInc.checked())
        return pos.coord(coord);
    else
        return abs.coord(coord);
}

void MainScreen::setCoord(int coord, double value)
{
    if (buttonInc.checked())
        pos.coord(coord) = value;
    else {
        float delta = value - abs.coord(coord);
        abs.coord(coord) = value;
        pos.coord(coord) += delta;
    }
}

