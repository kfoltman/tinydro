#pragma once

#include "widgets.h"
#include <cassert>
#include <optional>
#include <sstream>

extern int last_fps_value_x10;

struct TouchscreenEvent;

class CalibrationScreen: public Screen
{
public:
    Label label;
    int phase{0};
    int x, y;
    void init();
    void loop();
    void startCalibration();
    bool calibrationDone() const;
    // bool renderLine(int line) override;
    void onEvent(const TouchscreenEvent *event) override;
};

