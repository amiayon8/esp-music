#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "epaper_driver.hpp"

enum class CanvasColor : uint8_t {
    Black = 0,
    White = 1,
    Inverse = 2
};

class EPaperCanvas {
public:
    static constexpr uint16_t Width = EPaperDriver::DisplayWidth;
    static constexpr uint16_t Height = EPaperDriver::DisplayHeight;
    static constexpr size_t BufferSize = EPaperDriver::BufferSizeBytes;

    EPaperCanvas();
    ~EPaperCanvas();

    bool allocateBuffer();
    void clear(CanvasColor color = CanvasColor::White);
    
    void setPixel(int16_t x, int16_t y, CanvasColor color);
    CanvasColor getPixel(int16_t x, int16_t y) const;

    void drawHorizontalLine(int16_t x, int16_t y, int16_t length, CanvasColor color);
    void drawVerticalLine(int16_t x, int16_t y, int16_t length, CanvasColor color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, CanvasColor color);

    void drawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, CanvasColor color);
    void fillRectangle(int16_t x, int16_t y, int16_t width, int16_t height, CanvasColor color);

    void drawRoundedRectangle(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, CanvasColor color);
    void fillRoundedRectangle(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, CanvasColor color);

    void drawCircle(int16_t centerX, int16_t centerY, int16_t radius, CanvasColor color);
    void fillCircle(int16_t centerX, int16_t centerY, int16_t radius, CanvasColor color);

    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t width, int16_t height, CanvasColor color, bool inverted = false);

    const uint8_t* getBuffer() const;
    uint8_t* getBufferMutable();

private:
    uint8_t* frameBuffer;
    bool allocatedInPsram;
};
