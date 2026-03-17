#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#ifndef DESKTOPSIM
#include "ili9488.h"
#endif

#define LINE_WIDTH 480
#define SCREEN_HEIGHT 320

using Pixel = uint16_t;

extern Pixel line_buffer[LINE_WIDTH];
extern int globalTime;

struct RGB565
{
    Pixel value;
    RGB565(Pixel _value) : value{_value} {}
    inline uint8_t r() const { return (value >> 11) << 3; }
    inline uint32_t r5() const { return (value >> 11); }
    inline uint8_t g() const { return ((value >> 5) & 0x3F ) << 2; }
    inline uint32_t g6() const { return (value >> 5) & 0x3F; }
    inline uint8_t b() const { return (value & 0x1F) << 3; }
    inline uint32_t b5() const { return value & 0x1F; }
};

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return b | (uint16_t{g} << 5) | (uint16_t{r} << 11);
}

static inline constexpr uint16_t rgb888(uint8_t r, uint8_t g, uint8_t b)
{
    return (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
}

static inline constexpr uint16_t grayscale(int y)
{
    return (y >> 3) | ((y >> 2) << 5) | ((y >> 3) << 11);
}

static inline constexpr uint8_t blend15(uint8_t a, uint8_t b, uint8_t t)
{
    return (a * (15U - t) + b * t) / 15U;
}

static inline constexpr int blend255(uint8_t a, uint8_t b, uint8_t t)
{
    return (a * (255U - t) + b * t) / 255U;
}

static inline void renderHorizLine(int sx, int ex, uint16_t bg)
{
    while(sx < ex)
        line_buffer[sx++] = bg;
}

static inline void renderHorizLineClipped(int pos_x, int sx, int ex, int sxClip, int exClip, uint16_t bg)
{
    sx = pos_x + std::max(sx, sxClip);
    ex = pos_x + std::min(ex, exClip);
    while(sx < ex)
        line_buffer[sx++] = bg;
}

struct Bitmap
{
    const uint8_t *data;
    uint16_t width, height;
};

struct MonochromeBitmap: public Bitmap
{
    MonochromeBitmap(const uint8_t *_data, uint16_t _width, uint16_t _height) {
        data = _data;
        width = _width;
        height = _height;
    }
};

struct Gray4Bitmap: public Bitmap
{
    Gray4Bitmap(const uint8_t *_data, uint16_t _width, uint16_t _height) {
        data = _data;
        width = _width;
        height = _height;
    }
};

class Display
{
public:
#ifndef DESKTOPSIM
    ILI9488LCD &lcd;
    Display(ILI9488LCD &_lcd) : lcd(_lcd) {}
#endif
    void fill(int line, int xs, int xe, Pixel colour);
    void copy(int line, int xs, int xe, const Pixel *data);
};