#include "buzzer.h"
#include "screen.h"
#include "touchscreen.h"
#include "droscreens.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

BEGIN_MENU(MainScreen, Functions)
    { "Bolt circle (PCD)", &MainScreen::boltCircle },
    { "Line (PLD)", &MainScreen::holeRow },
    { "Arc", &MainScreen::arc, &MainScreen::getValue },
END_MENU(MainScreen, Functions)

BEGIN_MENU(MainScreen, LineType)
    { "Start, end, #", &MainScreen::holeRowSEC },
    { "Pt, angle, len, #", &MainScreen::holeRowSALC },
    { "Pt, angle, spc, #", &MainScreen::holeRowSASC },
END_MENU(MainScreen, LineType)

BEGIN_MENU(MainScreen, LineTypeSEC)
    { "Start pos", MainScreen::dummyItem(&MainScreen::menuLineTypeSEC) },
    { "End pos", MainScreen::dummyItem(&MainScreen::menuLineTypeSEC) },
    { "# Holes", MainScreen::dummyItem(&MainScreen::menuLineTypeSEC) },
END_MENU(MainScreen, LineTypeSEC)

BEGIN_MENU(MainScreen, LineTypeSALC)
    { "Start pos", MainScreen::dummyItem(&MainScreen::menuLineTypeSALC) },
    { "Angle", MainScreen::dummyItem(&MainScreen::menuLineTypeSALC) },
    { "Spacing", MainScreen::dummyItem(&MainScreen::menuLineTypeSALC) },
    { "# Holes", MainScreen::dummyItem(&MainScreen::menuLineTypeSALC) },
END_MENU(MainScreen, LineTypeSALC)

BEGIN_MENU(MainScreen, LineTypeSASC)
    { "Start pos", MainScreen::dummyItem(&MainScreen::menuLineTypeSASC) },
    { "Angle", MainScreen::dummyItem(&MainScreen::menuLineTypeSASC) },
    { "Spacing", MainScreen::dummyItem(&MainScreen::menuLineTypeSASC) },
    { "# Holes", MainScreen::dummyItem(&MainScreen::menuLineTypeSASC) },
END_MENU(MainScreen, LineTypeSASC)

void MainScreen::init()
{
#if 0
    pos_x = LINE_WIDTH / 4;
    width = LINE_WIDTH / 2;
    pos_y = SCREEN_HEIGHT / 4;
    height = SCREEN_HEIGHT / 2;
#endif
#if 0
    flags |= WF_BORDER;
#endif
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

    addMenu(menuFunctions);
    addMenu(menuLineType);
    addMenu(menuLineTypeSEC);
    addMenu(menuLineTypeSALC);
    addMenu(menuLineTypeSASC);

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

    buttonFunc.action = [this]() {
        if (menuFunctions.isShown())
            menuFunctions.hide();
        else
            menuFunctions.show();
    };
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

void MainScreen::addMenu(SimpleMenu<MainScreen> &widget)
{
    widget.z_index = 5;
    widget.flags |= WF_HIDDEN;
    add(&widget);
}

void MainScreen::hideMenu(Widget &widget)
{
    widget.hide();
}

void MainScreen::popupMenu(SimpleMenu<MainScreen> &widget)
{
    widget.scrollToTop();
    widget.show();
}

bool MainScreen::getValue(char *buf, unsigned size)
{
    snprintf(buf, size, "Value");
    return true;
}

void MainScreen::boltCircle()
{
    hideMenu(menuFunctions);
}

void MainScreen::holeRow()
{
    hideMenu(menuFunctions);
    popupMenu(menuLineType);
}

void MainScreen::arc()
{
    hideMenu(menuFunctions);
}

void MainScreen::holeRowSEC()
{
    hideMenu(menuLineType);
    popupMenu(menuLineTypeSEC);
}

void MainScreen::holeRowSALC()
{
    hideMenu(menuLineType);
    popupMenu(menuLineTypeSALC);
}

void MainScreen::holeRowSASC()
{
    hideMenu(menuLineType);
    popupMenu(menuLineTypeSASC);
}
