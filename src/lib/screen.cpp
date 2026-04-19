#include "buzzer.h"
#include "screen.h"
#include "touchscreen.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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
    //debug_label.z_index = 1;
    add(&debug_label);

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

void CalibrationScreen::init()
{
    label.pos_x = 0;
    label.pos_y = 0;
    label.width = LINE_WIDTH;
    label.setFont(&font_small);
    label.setText("Touch the cross mark");
    label.height = label.text.height();
    label.flags = WF_ALIGN_CENTRE;
    add(&label);
}

void CalibrationScreen::loop()
{
}

#if 0
bool CalibrationScreen::renderLine(int line)
{
    if (!Screen::renderLine(line))
    {
        if (flags & WF_UNCHANGED)
            return false;
        renderHorizLine(0, LINE_WIDTH, label.bg());
    }
    int crossx = 0, crossy = 0;
    switch(phase) {
    case 0: 
        crossx = 10; 
        crossy = 10;
        break;
    case 1: 
        crossx = LINE_WIDTH - 30; 
        crossy = 10;
        break;
    case 2: 
        crossx = LINE_WIDTH - 30; 
        crossy = 320 - 30;
        break;
    case 3: 
        crossx = 10; 
        crossy = 320 - 30;
        break;
    case 4:
        return true;
    case 5:
        crossx = x; 
        crossy = y;
        break;
    }
    uint16_t col = 0;
    if (line >= crossy - 10 && line <= crossy + 10) {
        #define PIXEL(x) if ((x) >= 0 && (x) < LINE_WIDTH) { line_buffer[(x)] = col; }
        int rel = line - crossy;
        PIXEL(crossx + rel);
        PIXEL(crossx + rel + 1);
        PIXEL(crossx - rel);
        PIXEL(crossx - rel - 1);
    }
    return true;
}
#endif

void CalibrationScreen::onEvent(const TouchscreenEvent *event)
{
    if (event->type == TouchscreenEvent::PRESS) {
        // char buf[255];
        // sprintf(buf, "X%d Y%d ZA%d ZB%d E%d", event->rawX, event->rawY, event->rawZ1, event->rawZ2, event->type);
        // label.setText(buf);
        // x = event->x;
        // y = event->y;
        buzzer.click();
        phase++;
        dirty();
    }
}

void CalibrationScreen::startCalibration()
{
    phase = 0;
}

bool CalibrationScreen::calibrationDone() const
{
    return phase == 4;
}
