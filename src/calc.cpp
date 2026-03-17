#ifdef ARDUINO
#include <avr/dtostrf.h>
#endif

#include <stdio.h>
#include <string.h>
#include "eval.h"
#include "calc.h"

using string = std::string;

uint16_t CalcDisplay::fg() const
{
    return ghost ? grayscale(128) : grayscale(224);
}

void CalcScreen::init()
{
    prompt.pos_x = 0;
    prompt.pos_y = 0;
    prompt.width = LINE_WIDTH - 60;
    prompt.height = font_small_bold.charHeight;
    prompt.setFont(&font_small_bold);
    prompt.setText("");
    add(&prompt);
    
    expression.clear();
    display.pos_x = 0;
    display.pos_y = font_small_bold.charHeight;
    display.width = LINE_WIDTH - 60;
    display.height = 60;
    display.flags = WF_BORDER | WF_INVERSE | WF_ALIGN_RIGHT;
    display.setFont(&font_sans);
    const char *button_text[4][6] = {
        {"7", "8", "9", CH_DIV, "(", "ESC"},
        {"4", "5", "6", CH_MUL, ")", CH_BACKSPACE},
        {"1", "2", "3", CH_SUB, CH_SQRT, "Fn"},
        {"+/-", "0", ".", CH_ADD, "a" CH_PWR, CH_ENTER},
    };
    add(&display);
    ac_button.pos_x = LINE_WIDTH - 60 + 4;
    ac_button.pos_y = display.pos_y;
    ac_button.width = 60 - 4;
    ac_button.height = display.height;
    ac_button.setText("AC");
    ac_button.action = [this]{clear();};
    add(&ac_button);
    
    int keypad_height = 320 - display.height - display.pos_y;
    int rows = 4, cols = 6;
    int xmargin = 5, ymargin = 3;
    int width = (LINE_WIDTH - ((cols - 1) * xmargin)) / cols;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            Button &b = buttons[y][x];
            b.pos_x = (LINE_WIDTH - width) * x / (cols - 1);
            b.pos_y = display.pos_y + display.height + keypad_height / rows * y + ymargin;
            b.width = width;
            b.height = keypad_height / rows - 2 * ymargin;
            b.setText(button_text[y][x]);
            b.action = [this, y, x]{onButton(y, x);};
            add(&b);
        }
    }
}

void CalcScreen::clear()
{
    expression.clear();
    display.ghost = true;
}

void CalcScreen::set(const std::string &text)
{
    expression = text;
    display.ghost = false;
}

void CalcScreen::setFloat(double value)
{
    display.ghost = true;
    char buf[256];
    snprintf(buf, sizeof(buf), "%f", value);
    expression = buf;
    string::size_type pos = expression.find('.');
    if (pos == string::npos)
        return;
    if (expression.find(pos, 'e') != string::npos)
        return;
    while(*expression.rbegin() == '0')
        expression.erase(expression.end() - 1);
    if(*expression.rbegin() == '.')
        expression.erase(expression.end() - 1);
}

void CalcScreen::ask(const std::string &prompt, double value)
{
    error_display.clear();
    prompt_text = prompt;
    setFloat(value);
}

void CalcScreen::loop()
{
    display.setText(expression[0] ? expression : "0");
    if (error_display.empty())
        prompt.setText(prompt_text.empty() ? std::string("Enter a value:") : prompt_text);
    else
        prompt.setText(error_display);
}

void CalcScreen::backspace()
{
    if (display.ghost) {
        expression.clear();
        return;
    }

    if (expression.length()) {
        expression.erase(expression.end() - 1);
    }
    error_display.clear();
}

void CalcScreen::onButton(int y, int x)
{
    const char *toAdd{};
    switch(y) {
    case 0:
        switch(x) {
            case 0: toAdd = "7"; break;
            case 1: toAdd = "8"; break;
            case 2: toAdd = "9"; break;
            case 3: toAdd = "/"; break;
            case 4: toAdd = "("; break;
            case 5: 
                if (onEsc)
                    onEsc();
                else
                    expression.clear();
                break;
        }
        break;
    case 1:
        switch(x) {
            case 0: toAdd = "4"; break;
            case 1: toAdd = "5"; break;
            case 2: toAdd = "6"; break;
            case 3: toAdd = "*"; break;
            case 4: toAdd = ")"; break;
            case 5: backspace(); break;
        }
        break;
    case 2:
        switch(x) {
            case 0: toAdd = "1"; break;
            case 1: toAdd = "2"; break;
            case 2: toAdd = "3"; break;
            case 3: 
                toAdd = "-";
                if (display.ghost && expression == "0") {
                    expression.clear();
                }
                break;
            case 4: toAdd = CH_SQRT; break;
        }
        break;
    case 3:
        switch(x) {
            case 1: toAdd = "0"; break;
            case 2: toAdd = "."; break;
            case 3: toAdd = "+"; break;
            case 4: toAdd = CH_PWR2; break;
            case 5: calculate(); break;
        }
        break;
    }
    if (toAdd) {
        if (display.ghost && toAdd && ((toAdd[0] >= '0' && toAdd[0] <= '9') || toAdd[0] == CH_SQRT[0] || toAdd[0] == '(')) {
            expression.clear();
        }
        display.ghost = false;
        error_display.clear();
        expression += toAdd;
    }
}

void CalcScreen::calculate()
{
    CalcEvaluator eval(expression.c_str());
    std::optional<double> value = eval.evaluate();
    if (!value.has_value())
    {
        error_display = eval.error;
        return;
    }
    if (onEnter) {
        error_display = onEnter(value.value());
    }
    else
        setFloat(value.value());
}

