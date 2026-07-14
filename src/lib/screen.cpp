#include "buzzer.h"
#include "screen.h"
#include "touchscreen.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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
