#pragma once

#include <cstdint>
#include <cstddef>
#include "epaper_canvas.hpp"

enum class FontSize {
    Small,
    Medium,
    Large,
    Title
};

struct FontGlyph {
    uint8_t width;
    uint8_t height;
    int8_t advance;
    const uint8_t* bitmap;
};

class FontEngine {
public:
    static int16_t drawCharacter(EPaperCanvas& canvas, int16_t x, int16_t y, char character, FontSize size, CanvasColor color);
    static int16_t drawString(EPaperCanvas& canvas, int16_t x, int16_t y, const char* text, FontSize size, CanvasColor color);
    static int16_t drawStringTruncated(EPaperCanvas& canvas, int16_t x, int16_t y, const char* text, int16_t maxWidth, FontSize size, CanvasColor color);
    static int16_t measureStringWidth(const char* text, FontSize size);
    static int16_t getFontHeight(FontSize size);
};
