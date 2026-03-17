#include <assert.h>
#include "bmpfont.h"

uint16_t line_buffer[LINE_WIDTH];

#define INVALID_COLOUR 0xF81F

struct AACache
{
    uint16_t colour[16];
    uint32_t fgx, bgx;
    AACache() {}
    inline void update(Pixel _fg, Pixel _bg)
    {
        if (_fg != colour[15] || _bg != colour[0])
        {
#if 0
            if (valid) {
                printf("Collision?\n");
            }
#endif
            colour[0] = _bg;
            colour[15] = _fg;
            fgx = ((_fg & 0xF800) << 8) | ((_fg & 0x07E0) << 4) | (_fg & 0x001F);
            bgx = ((_bg & 0xF800) << 8) | ((_bg & 0x07E0) << 4) | (_bg & 0x001F);
            for (unsigned value = 1U; value < 15U; ++value) {
                unsigned value_scaled = (value > 7) ? (value + 1) : value;
                uint32_t mix = fgx * value_scaled + bgx * (16 - value_scaled);
                colour[value] = ((mix >> 12) & 0xF800) | ((mix >> 8) & 0x07E0) | ((mix >> 4) & 31);
            }
        }
    }
    inline Pixel get(uint8_t value) {
        return colour[value];
    }
    ~AACache() {
    }
};

void renderTextSpan(int line, int sx, int clip, int ex, const TextGlyphs &glyphs, uint16_t fg, uint32_t bg)
{
    static AACache aacaches[64];
    uint16_t hashv = (fg << 4) ^ bg;
    uint16_t hash = (hashv - (hashv >> 9)) & 63;
    AACache &aacache = aacaches[hash];
    aacache.update(fg, bg);
    const Font &font = *glyphs.getFont();
    const std::string &text = glyphs.getText();
    unsigned int bmpWidth = font.xs[font.charCount];
    unsigned int glyphRowBytes = (font.bitsPerPixel == 4) ? ((bmpWidth + 1) >> 1) : ((bmpWidth + 7) >> 3);
    const uint8_t *glyphLine = font.bitmap->data + line * glyphRowBytes;
    assert(sx >= 0);
    if (clip < 0) {
        // Left pad
        int skip = -clip;
        int nex = std::min(sx + skip, ex);
        renderHorizLine(sx, nex, bg);
        if (nex >= ex)
            return;
        sx = nex;
        clip = 0;
    }
    if (line >= 0 && line < font.charHeight) {
        Pixel *output = line_buffer + std::max(0, sx);
        std::string::size_type i = 0;
        // Clip whole characters quickly
        for (; clip > 0 && i < text.length(); ++i) {
            int glyphIndex = ((uint8_t)text[i]) - font.firstChar;
            int charWidth = font.spaceWidth;
            if (glyphIndex >= 0 && glyphIndex < font.charCount) {
                charWidth = font.xs[glyphIndex + 1] - font.xs[glyphIndex];
            }
            if (clip < charWidth)
                break;
            clip -= charWidth;
        }
        int maxWidth = &line_buffer[ex] - output;
        for (; i < text.length() && maxWidth > 0; ++i) {
            int glyphIndex = ((uint8_t)text[i]) - font.firstChar;
            int glyphWidth = font.anyCharWidth((uint8_t)text[i]);
            int pixelsToDraw = std::min<int>(maxWidth, std::max<int>(0, glyphWidth - clip));
            if (glyphIndex < 0 || glyphIndex >= font.charCount) {
                for (int x = 0; x < pixelsToDraw; x++) {
                    *output++ = bg;
                }
            } else {
                const int ox = font.xs[glyphIndex] + clip;
                if (font.bitsPerPixel == 4) {
                    for (int x = 0; x < pixelsToDraw; x++) {
                        *output++ = aacache.get((glyphLine[(ox + x) >> 1] >> ((ox + x) & 1 ? 0 : 4)) & 15);
                    }
                } else {
                    for (int x = 0; x < pixelsToDraw; x++) {
                        *output++ = (128 & (glyphLine[((ox + x) >> 3)] << ((ox + x) & 7))) ? fg : bg;
                    }
                }
            }
            clip = std::max(0, clip - glyphWidth);
            maxWidth -= pixelsToDraw;
        }
        sx = output - line_buffer;
    }
    renderHorizLine(std::max(sx, 0), ex, bg);
}

void TextGlyphs::setFont(const Font *_font)
{
    if (font == _font)
        return;
    font = _font;
    totalWidth = 0;
}

bool TextGlyphs::setText(const std::string &_text)
{
    if (text == _text)
        return false;
    text = _text;
    totalWidth = 0;
    return true;
}

void TextGlyphs::updateWidth()
{
    totalWidth = 0;
    for (std::string::size_type i = 0; i < text.size(); ++i) {
        int glyphIndex = text[i] - font->firstChar;
        if (glyphIndex >= 0 && glyphIndex < font->charCount)
            totalWidth += font->xs[glyphIndex + 1] - font->xs[glyphIndex];
        else
            totalWidth += font->spaceWidth;
    }
}

#ifndef DESKTOPSIM

void Display::fill(int line, int xs, int xe, Pixel colour)
{
    volatile uint16_t *lcd_data = lcd.startPixels(xs, line, xe - 1, line);
    for (int x = xs; x < xe; ++x)
        *lcd_data = colour;
}

void Display::copy(int line, int xs, int xe, const Pixel *data)
{
    volatile uint16_t *lcd_data = lcd.startPixels(xs, line, xe - 1, line);
    for (int x = xs; x < xe; ++x)
        *lcd_data = data[x - xs];
}

#endif

