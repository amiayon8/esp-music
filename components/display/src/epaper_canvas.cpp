#include "epaper_canvas.hpp"
#include "esp_heap_caps.h"
#include <cstdlib>
#include <cmath>

EPaperCanvas::EPaperCanvas()
    : frameBuffer(nullptr), allocatedInPsram(false) {
}

EPaperCanvas::~EPaperCanvas() {
    if (frameBuffer != nullptr) {
        free(frameBuffer);
        frameBuffer = nullptr;
    }
}

bool EPaperCanvas::allocateBuffer() {
    if (frameBuffer != nullptr) {
        return true;
    }

    frameBuffer = static_cast<uint8_t*>(heap_caps_malloc(BufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (frameBuffer != nullptr) {
        allocatedInPsram = true;
    } else {
        frameBuffer = static_cast<uint8_t*>(malloc(BufferSize));
        allocatedInPsram = false;
    }

    if (frameBuffer != nullptr) {
        clear(CanvasColor::White);
        return true;
    }
    return false;
}

void EPaperCanvas::clear(CanvasColor color) {
    if (frameBuffer == nullptr) {
        return;
    }
    uint8_t fillByte = (color == CanvasColor::Black) ? 0x00 : 0xFF;
    std::memset(frameBuffer, fillByte, BufferSize);
}

void EPaperCanvas::setPixel(int16_t x, int16_t y, CanvasColor color) {
    if (frameBuffer == nullptr || x < 0 || x >= Width || y < 0 || y >= Height) {
        return;
    }

    size_t byteIndex = (static_cast<size_t>(x) / 8) + (static_cast<size_t>(y) * (Width / 8));
    uint8_t bitMask = 0x80 >> (x % 8);

    if (color == CanvasColor::Black) {
        frameBuffer[byteIndex] &= ~bitMask;
    } else if (color == CanvasColor::White) {
        frameBuffer[byteIndex] |= bitMask;
    } else if (color == CanvasColor::Inverse) {
        frameBuffer[byteIndex] ^= bitMask;
    }
}

CanvasColor EPaperCanvas::getPixel(int16_t x, int16_t y) const {
    if (frameBuffer == nullptr || x < 0 || x >= Width || y < 0 || y >= Height) {
        return CanvasColor::White;
    }

    size_t byteIndex = (static_cast<size_t>(x) / 8) + (static_cast<size_t>(y) * (Width / 8));
    uint8_t bitMask = 0x80 >> (x % 8);

    return (frameBuffer[byteIndex] & bitMask) ? CanvasColor::White : CanvasColor::Black;
}

void EPaperCanvas::drawHorizontalLine(int16_t x, int16_t y, int16_t length, CanvasColor color) {
    if (y < 0 || y >= Height || length <= 0) {
        return;
    }

    int16_t startX = (x < 0) ? 0 : x;
    int16_t endX = x + length - 1;
    if (endX >= Width) {
        endX = Width - 1;
    }

    for (int16_t currentX = startX; currentX <= endX; ++currentX) {
        setPixel(currentX, y, color);
    }
}

void EPaperCanvas::drawVerticalLine(int16_t x, int16_t y, int16_t length, CanvasColor color) {
    if (x < 0 || x >= Width || length <= 0) {
        return;
    }

    int16_t startY = (y < 0) ? 0 : y;
    int16_t endY = y + length - 1;
    if (endY >= Height) {
        endY = Height - 1;
    }

    for (int16_t currentY = startY; currentY <= endY; ++currentY) {
        setPixel(x, currentY, color);
    }
}

void EPaperCanvas::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, CanvasColor color) {
    int16_t deltaX = std::abs(x1 - x0);
    int16_t deltaY = -std::abs(y1 - y0);
    int16_t stepX = (x0 < x1) ? 1 : -1;
    int16_t stepY = (y0 < y1) ? 1 : -1;
    int16_t error = deltaX + deltaY;

    while (true) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int16_t errorDouble = 2 * error;
        if (errorDouble >= deltaY) {
            error += deltaY;
            x0 += stepX;
        }
        if (errorDouble <= deltaX) {
            error += deltaX;
            y0 += stepY;
        }
    }
}

void EPaperCanvas::drawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, CanvasColor color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    drawHorizontalLine(x, y, width, color);
    drawHorizontalLine(x, y + height - 1, width, color);
    drawVerticalLine(x, y, height, color);
    drawVerticalLine(x + width - 1, y, height, color);
}

void EPaperCanvas::fillRectangle(int16_t x, int16_t y, int16_t width, int16_t height, CanvasColor color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    for (int16_t currentY = y; currentY < y + height; ++currentY) {
        drawHorizontalLine(x, currentY, width, color);
    }
}

void EPaperCanvas::drawRoundedRectangle(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, CanvasColor color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (radius <= 0) {
        drawRectangle(x, y, width, height, color);
        return;
    }

    if (radius > width / 2) {
        radius = width / 2;
    }
    if (radius > height / 2) {
        radius = height / 2;
    }

    drawHorizontalLine(x + radius, y, width - 2 * radius, color);
    drawHorizontalLine(x + radius, y + height - 1, width - 2 * radius, color);
    drawVerticalLine(x, y + radius, height - 2 * radius, color);
    drawVerticalLine(x + width - 1, y + radius, height - 2 * radius, color);

    int16_t offsetX = 0;
    int16_t offsetY = radius;
    int16_t decision = 3 - 2 * radius;

    while (offsetY >= offsetX) {
        setPixel(x + radius - offsetX, y + radius - offsetY, color);
        setPixel(x + width - 1 - radius + offsetX, y + radius - offsetY, color);
        setPixel(x + radius - offsetX, y + height - 1 - radius + offsetY, color);
        setPixel(x + width - 1 - radius + offsetX, y + height - 1 - radius + offsetY, color);

        setPixel(x + radius - offsetY, y + radius - offsetX, color);
        setPixel(x + width - 1 - radius + offsetY, y + radius - offsetX, color);
        setPixel(x + radius - offsetY, y + height - 1 - radius + offsetX, color);
        setPixel(x + width - 1 - radius + offsetY, y + height - 1 - radius + offsetX, color);

        if (decision < 0) {
            decision += 4 * offsetX + 6;
        } else {
            decision += 4 * (offsetX - offsetY) + 10;
            offsetY--;
        }
        offsetX++;
    }
}

void EPaperCanvas::fillRoundedRectangle(int16_t x, int16_t y, int16_t width, int16_t height, int16_t radius, CanvasColor color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (radius <= 0) {
        fillRectangle(x, y, width, height, color);
        return;
    }

    if (radius > width / 2) {
        radius = width / 2;
    }
    if (radius > height / 2) {
        radius = height / 2;
    }

    fillRectangle(x + radius, y, width - 2 * radius, height, color);

    int16_t offsetX = 0;
    int16_t offsetY = radius;
    int16_t decision = 3 - 2 * radius;

    while (offsetY >= offsetX) {
        drawVerticalLine(x + radius - offsetX, y + radius - offsetY, offsetY * 2, color);
        drawVerticalLine(x + width - 1 - radius + offsetX, y + radius - offsetY, offsetY * 2, color);
        drawVerticalLine(x + radius - offsetY, y + radius - offsetX, offsetX * 2, color);
        drawVerticalLine(x + width - 1 - radius + offsetY, y + radius - offsetX, offsetX * 2, color);

        if (decision < 0) {
            decision += 4 * offsetX + 6;
        } else {
            decision += 4 * (offsetX - offsetY) + 10;
            offsetY--;
        }
        offsetX++;
    }
}

void EPaperCanvas::drawCircle(int16_t centerX, int16_t centerY, int16_t radius, CanvasColor color) {
    int16_t x = 0;
    int16_t y = radius;
    int16_t decision = 3 - 2 * radius;

    while (y >= x) {
        setPixel(centerX + x, centerY + y, color);
        setPixel(centerX - x, centerY + y, color);
        setPixel(centerX + x, centerY - y, color);
        setPixel(centerX - x, centerY - y, color);
        setPixel(centerX + y, centerY + x, color);
        setPixel(centerX - y, centerY + x, color);
        setPixel(centerX + y, centerY - x, color);
        setPixel(centerX - y, centerY - x, color);

        if (decision < 0) {
            decision += 4 * x + 6;
        } else {
            decision += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void EPaperCanvas::fillCircle(int16_t centerX, int16_t centerY, int16_t radius, CanvasColor color) {
    int16_t x = 0;
    int16_t y = radius;
    int16_t decision = 3 - 2 * radius;

    while (y >= x) {
        drawHorizontalLine(centerX - x, centerY + y, 2 * x + 1, color);
        drawHorizontalLine(centerX - x, centerY - y, 2 * x + 1, color);
        drawHorizontalLine(centerX - y, centerY + x, 2 * y + 1, color);
        drawHorizontalLine(centerX - y, centerY - x, 2 * y + 1, color);

        if (decision < 0) {
            decision += 4 * x + 6;
        } else {
            decision += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void EPaperCanvas::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t width, int16_t height, CanvasColor color, bool inverted) {
    if (bitmap == nullptr || width <= 0 || height <= 0) {
        return;
    }

    int16_t rowBytes = (width + 7) / 8;

    for (int16_t currentY = 0; currentY < height; ++currentY) {
        for (int16_t currentX = 0; currentX < width; ++currentX) {
            size_t byteIndex = (currentY * rowBytes) + (currentX / 8);
            uint8_t bitMask = 0x80 >> (currentX % 8);
            bool isPixelActive = (bitmap[byteIndex] & bitMask) != 0;

            if (inverted) {
                isPixelActive = !isPixelActive;
            }

            if (isPixelActive) {
                setPixel(x + currentX, y + currentY, color);
            }
        }
    }
}

const uint8_t* EPaperCanvas::getBuffer() const {
    return frameBuffer;
}

uint8_t* EPaperCanvas::getBufferMutable() {
    return frameBuffer;
}
