#pragma once

#include <cstdint>

static volatile uint16_t *const lcd_ctl = reinterpret_cast<volatile uint16_t *>(0x60000000);
static volatile uint16_t *const lcd_data = reinterpret_cast<volatile uint16_t *>(0x60020000);

struct FontData
{
    uint8_t charWidth, charHeight, firstChar;
    const uint8_t *bits;
};

class ILI9488LCD
{
public:
    static constexpr unsigned int WIDTH{480};
    static constexpr unsigned int HEIGHT{320};

    void init();
    volatile uint16_t *startPixels(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
    void drawFontSpan(unsigned int clip, unsigned int width, const char *text, int len, const FontData &font, int fontLine);
};

extern FontData font_8x16, font_Grotesk_16x32, font_Grotesk_24x48;
