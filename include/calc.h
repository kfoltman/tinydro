#pragma once

#include "widgets.h"

class CalcDisplay: public Label
{
public:
    bool ghost{false};
    virtual uint16_t fg() const;
};

class CalcScreen: public Screen
{
protected:
    Label prompt;
    CalcDisplay display;
    Button buttons[4][6];
    Button ac_button;
    std::string expression;
    std::string error_display;

public:
    std::string prompt_text;
    std::function<std::string(float)> onEnter;
    std::function<void()> onEsc;
    
public:
    void init();
    void loop();
    void onButton(int y, int x);
    void set(const std::string &text);
    void setFloat(double value);
    void ask(const std::string &prompt, double value);
    void backspace();
    void calculate();
    void clear();
};

