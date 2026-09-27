#include "bitmap_renderer.hpp"
#include <vector>
#include <algorithm>

void BitmapRenderer::ditherGrayscaleTo1Bit(const uint8_t* grayscaleInput, uint8_t* monochromeOutput,
                                           uint16_t width, uint16_t height) {
    if (grayscaleInput == nullptr || monochromeOutput == nullptr || width == 0 || height == 0) {
        return;
    }

    std::vector<int16_t> errorBuffer(width * height);
    for (size_t index = 0; index < width * height; ++index) {
        errorBuffer[index] = static_cast<int16_t>(grayscaleInput[index]);
    }

    size_t outputStride = (width + 7) / 8;
    for (size_t index = 0; index < outputStride * height; ++index) {
        monochromeOutput[index] = 0xFF;
    }

    for (uint16_t currentY = 0; currentY < height; ++currentY) {
        for (uint16_t currentX = 0; currentX < width; ++currentX) {
            size_t pixelIndex = currentY * width + currentX;
            int16_t oldPixel = errorBuffer[pixelIndex];
            oldPixel = std::max<int16_t>(0, std::min<int16_t>(255, oldPixel));

            uint8_t newPixel = (oldPixel < 128) ? 0 : 255;
            int16_t quantizationError = oldPixel - newPixel;

            if (newPixel == 0) {
                size_t byteIndex = (currentY * outputStride) + (currentX / 8);
                uint8_t bitMask = 0x80 >> (currentX % 8);
                monochromeOutput[byteIndex] &= ~bitMask;
            }

            if (currentX + 1 < width) {
                errorBuffer[pixelIndex + 1] += (quantizationError * 7) / 16;
            }
            if (currentY + 1 < height) {
                if (currentX > 0) {
                    errorBuffer[pixelIndex + width - 1] += (quantizationError * 3) / 16;
                }
                errorBuffer[pixelIndex + width] += (quantizationError * 5) / 16;
                if (currentX + 1 < width) {
                    errorBuffer[pixelIndex + width + 1] += (quantizationError * 1) / 16;
                }
            }
        }
    }
}

void BitmapRenderer::renderArtwork(EPaperCanvas& canvas, int16_t x, int16_t y,
                                   const uint8_t* monochromeBitmap, uint16_t width, uint16_t height) {
    if (monochromeBitmap == nullptr) {
        renderDefaultArtwork(canvas, x, y, width);
        return;
    }
    canvas.drawBitmap(x, y, monochromeBitmap, width, height, CanvasColor::Black, false);
}

void BitmapRenderer::renderDefaultArtwork(EPaperCanvas& canvas, int16_t x, int16_t y, uint16_t size) {
    canvas.drawRectangle(x, y, size, size, CanvasColor::Black);
    int16_t centerCoordinate = size / 2;
    int16_t centerX = x + centerCoordinate;
    int16_t centerY = y + centerCoordinate;
    int16_t outerRadius = (size / 2) - 4;

    canvas.drawCircle(centerX, centerY, outerRadius, CanvasColor::Black);
    canvas.drawCircle(centerX, centerY, outerRadius - 8, CanvasColor::Black);
    canvas.drawCircle(centerX, centerY, outerRadius - 16, CanvasColor::Black);
    canvas.fillCircle(centerX, centerY, 8, CanvasColor::Black);
    canvas.fillCircle(centerX, centerY, 3, CanvasColor::White);
}
