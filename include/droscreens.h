#pragma once

#include "widgets.h"
#include <cassert>
#include <optional>
#include <sstream>

struct Coords
{
    int x{0}, y{0}, z{0};
    int &coord(int index) {
        static int error = 0xdeadface;
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
        assert(false && "Index = 0..2");
        return error;
    }
    int coord(int index) const {
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
        assert(false && "Index = 0..2");
        return 0;
    }
};

class TestMenu: public VerticalMenu
{
public:
    TestMenu(int16_t _pos_x, int16_t _pos_y, uint16_t _width, uint16_t _height)
    : VerticalMenu(_pos_x, _pos_y, _width, _height) {}
    const char *item(int index) override {
        switch(index) {
        case 0: return "Bolt circle";
        case 1: return "Line/grid";
        case 2: return "Foo";
        case 3: return "Bar";
        case 4: return "Baz";
        default: return nullptr;
        }
    }
    void activate(int index) override {
        // Do nothing
    }
};

class MainScreen: public Screen
{
public:
    static constexpr int margin = 4;
    static constexpr int x_coord_label = 0;
    static constexpr int width_coord_label = 30;
    static constexpr int x_coord_display = width_coord_label + margin;
    static constexpr int width_coord_display = 150;
    static constexpr int x_zero_button = x_coord_display + width_coord_display + margin;
    static constexpr int width_zero_button = 40;
    static constexpr int x_right_part = x_zero_button + width_zero_button;
    static constexpr int height_coord_row = 60;
    static constexpr int y_x_axis = margin;
    static constexpr int y_y_axis = margin + height_coord_row + margin;
    static constexpr int y_z_axis = margin + 2 * height_coord_row + 2 * margin;
    
    static constexpr int num_buttons = 5;
    static constexpr int height_buttons = 80;
    static constexpr int y_buttons = 320 - margin - height_buttons;
    static constexpr int width_button = (LINE_WIDTH - margin) / num_buttons - margin;

    static constexpr inline int16_t x_button(uint16_t index) {
        return margin + (width_button + margin) * index;
    }

    Coords pos, abs;

    Label x_label{x_coord_label, y_x_axis, width_coord_label, height_coord_row, WF_ALIGN_RIGHT, "X"};
    Label x_coord{x_coord_display, y_x_axis, width_coord_display, height_coord_row, WF_BORDER | WF_ALIGN_RIGHT | WF_INVERSE, ""};
    Button x_zero{x_zero_button, y_x_axis, width_zero_button, height_coord_row, "0"};

    Label y_label{x_coord_label, y_y_axis, width_coord_label, height_coord_row, WF_ALIGN_RIGHT, "Y"};
    Label y_coord{x_coord_display, y_y_axis, width_coord_display, height_coord_row, WF_BORDER | WF_ALIGN_RIGHT | WF_INVERSE, ""};
    Button y_zero{x_zero_button, y_y_axis, width_zero_button, height_coord_row, "0"};

    Label z_label{x_coord_label, y_z_axis, width_coord_label, height_coord_row, WF_ALIGN_RIGHT, "Z"};
    Label z_coord{x_coord_display, y_z_axis, width_coord_display, height_coord_row, WF_BORDER | WF_ALIGN_RIGHT | WF_INVERSE, ""};
    Button z_zero{x_zero_button, y_z_axis, width_zero_button, height_coord_row, "0"};

    //ScrollLabel debug_label{2, y_z_axis + height_coord_row + 2, LINE_WIDTH - 4, 32, WF_BORDER, "debug"};
    //ScrollLabel debug_label{30, 40, LINE_WIDTH - 2 * 30, 64, WF_BORDER, "debug"};
    Label debug_label{LINE_WIDTH / 2, 40, LINE_WIDTH / 2, 64, WF_BORDER, "debug"};

    ToggleButton buttonInc{x_button(0), y_buttons, width_button, height_buttons, "ABS", "INC"};
    ToggleButton buttonInch{x_button(1), y_buttons, width_button, height_buttons, "mm", "inch"};
    Button buttonMem{x_button(2), y_buttons, width_button, height_buttons, "MEM"};
    Button buttonFunc{x_button(3), y_buttons, width_button, height_buttons, "FUNC"};
    Button buttonSetup{x_button(4), y_buttons, width_button, height_buttons, "SETUP"};
    
    TestMenu menu{x_right_part + 10, 50, LINE_WIDTH - x_right_part - 20, 150};
    WidgetContainer menuScroll;

    void init();
    void loop();
    
    void formatCoords();
    void formatCoord(Label &display, int value);

    int fromDisplay(double value) const;
    double toDisplay(int micrometers) const;
    
    double getCoord(int coord) const;
    void setCoord(int coord, double value);
};

