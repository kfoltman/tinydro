#pragma once

#include <cstdint>
#include <string>
#include "gfx.h"

struct Font
{
    uint8_t charWidth, charHeight;
    uint8_t firstChar, charCount;
    uint8_t spaceWidth, bitsPerPixel;
    const uint16_t *xs;
    Bitmap *bitmap;
    inline uint32_t anyCharWidth(uint8_t ch) const { return (ch >= firstChar && ch < firstChar + charCount) ? xs[ch + 1 - firstChar] - xs[ch - firstChar] : spaceWidth; }
    inline uint32_t textWidth(const std::string &text) const;
};

uint32_t Font::textWidth(const std::string &text) const {
    uint32_t width = 0;
    for (uint8_t ch: text) {
        unsigned glyphIndex = ch - firstChar;
        if (glyphIndex < charCount)
            width += xs[glyphIndex + 1] - xs[glyphIndex];
        else {
            width += spaceWidth;
        }
    }
    return width;
}

extern Font font_lato;
extern Font font_sans;
extern Font font_serif;
extern Font font_small;
extern Font font_small_bold;

class TextGlyphs
{
    const Font *font;
    std::string text;
    int totalWidth;
public:
    TextGlyphs() : font{}, totalWidth{} {}
    TextGlyphs(const std::string &_text, const Font *_font) : font{_font}, text{_text}, totalWidth{} {}
    TextGlyphs(const TextGlyphs &other) = default;
    const Font *getFont() const { return font; }
    const std::string &getText() const { return text; }
    int width() { if (!totalWidth && !text.empty() && font) updateWidth(); return totalWidth; }
    int height() const { return font ? font->charHeight : 0; }
    void setFont(const Font *_font);
    bool setText(const std::string &_text);
    void updateWidth();
};

extern void renderTextSpan(int line, int sx, int clip, int ex, const TextGlyphs &text, uint16_t fg, uint32_t bg);
