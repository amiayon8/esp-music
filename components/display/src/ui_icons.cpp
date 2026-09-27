#include "ui_icons.hpp"
#include <algorithm>

void UiIcons::drawBluetooth(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    canvas.drawVerticalLine(x + 4, y, 13, color);
    canvas.drawLine(x + 4, y + 6, x + 8, y + 2, color);
    canvas.drawLine(x + 8, y + 2, x + 4, y, color);
    canvas.drawLine(x + 4, y + 6, x + 8, y + 10, color);
    canvas.drawLine(x + 8, y + 10, x + 4, y + 12, color);
    canvas.drawLine(x + 1, y + 3, x + 7, y + 9, color);
    canvas.drawLine(x + 1, y + 9, x + 7, y + 3, color);
}

void UiIcons::drawBatteryCapsule(EPaperCanvas& canvas, int16_t x, int16_t y, uint8_t percentage, CanvasColor color) {
    int16_t capsuleWidth = 24;
    int16_t capsuleHeight = 11;
    canvas.drawRoundedRectangle(x, y, capsuleWidth, capsuleHeight, 4, color);

    uint8_t clampedPercentage = std::min<uint8_t>(100, percentage);
    int16_t fillWidth = ((capsuleWidth - 4) * clampedPercentage) / 100;
    if (fillWidth > 0) {
        canvas.fillRoundedRectangle(x + 2, y + 2, fillWidth, capsuleHeight - 4, 2, color);
    }
}

void UiIcons::drawSpeaker(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    canvas.drawVerticalLine(x, y + 2, 4, color);
    canvas.drawVerticalLine(x + 1, y + 2, 4, color);
    canvas.drawLine(x + 1, y + 2, x + 4, y, color);
    canvas.drawLine(x + 1, y + 5, x + 4, y + 7, color);
    canvas.drawVerticalLine(x + 4, y, 8, color);

    canvas.drawLine(x + 6, y + 1, x + 7, y + 2, color);
    canvas.drawLine(x + 7, y + 2, x + 7, y + 5, color);
    canvas.drawLine(x + 7, y + 5, x + 6, y + 6, color);

    canvas.drawLine(x + 8, y, x + 10, y + 2, color);
    canvas.drawLine(x + 10, y + 2, x + 10, y + 5, color);
    canvas.drawLine(x + 10, y + 5, x + 8, y + 7, color);
}

void UiIcons::drawVolumeBar(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t height, uint8_t volumePercentage, CanvasColor color) {
    canvas.drawVerticalLine(x, y, height, color);
    uint8_t clampedPercentage = std::min<uint8_t>(100, volumePercentage);
    int16_t fillHeight = (height * clampedPercentage) / 100;
    if (fillHeight > 0) {
        int16_t startY = y + height - fillHeight;
        canvas.drawVerticalLine(x - 1, startY, fillHeight, color);
        canvas.drawVerticalLine(x, startY, fillHeight, color);
        canvas.drawVerticalLine(x + 1, startY, fillHeight, color);
    }
}

void UiIcons::drawPlayCircle(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t radius, CanvasColor color) {
    canvas.fillCircle(x, y, radius, color);
    CanvasColor interiorColor = (color == CanvasColor::Black) ? CanvasColor::White : CanvasColor::Black;

    int16_t triangleLeft = x - (radius / 3);
    int16_t triangleRight = x + (radius / 2);
    int16_t triangleTop = y - (radius / 2);
    int16_t triangleBottom = y + (radius / 2);

    for (int16_t currentX = triangleLeft; currentX <= triangleRight; ++currentX) {
        int16_t halfSpan = ((currentX - triangleLeft) * (triangleBottom - triangleTop)) / (2 * (triangleRight - triangleLeft + 1));
        canvas.drawVerticalLine(currentX, y - halfSpan, 2 * halfSpan + 1, interiorColor);
    }
}

void UiIcons::drawPauseCircle(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t radius, CanvasColor color) {
    canvas.fillCircle(x, y, radius, color);
    CanvasColor interiorColor = (color == CanvasColor::Black) ? CanvasColor::White : CanvasColor::Black;

    int16_t barWidth = radius / 3;
    int16_t barHeight = radius;
    int16_t barSpacing = radius / 4;

    canvas.fillRectangle(x - barSpacing - barWidth, y - (barHeight / 2), barWidth, barHeight, interiorColor);
    canvas.fillRectangle(x + barSpacing, y - (barHeight / 2), barWidth, barHeight, interiorColor);
}

void UiIcons::drawSdCardIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    int16_t cardWidth = 56;
    int16_t cardHeight = 64;

    canvas.drawLine(x - 10, y - 6, x - 2, y + 2, color);
    canvas.drawLine(x - 10, y - 5, x - 1, y + 4, color);
    canvas.drawLine(x + cardWidth + 10, y - 6, x + cardWidth + 2, y + 2, color);
    canvas.drawLine(x + cardWidth + 10, y - 5, x + cardWidth + 1, y + 4, color);

    canvas.drawLine(x - 10, y + cardHeight + 6, x - 2, y + cardHeight - 2, color);
    canvas.drawLine(x - 10, y + cardHeight + 5, x - 1, y + cardHeight - 4, color);
    canvas.drawLine(x + cardWidth + 10, y + cardHeight + 6, x + cardWidth + 2, y + cardHeight - 2, color);
    canvas.drawLine(x + cardWidth + 10, y + cardHeight + 5, x + cardWidth + 1, y + cardHeight - 4, color);

    canvas.fillRoundedRectangle(x, y, cardWidth, cardHeight, 6, color);
    canvas.fillRectangle(x + cardWidth - 14, y, 14, 14, CanvasColor::White);
    canvas.drawLine(x + cardWidth - 14, y, x + cardWidth, y + 14, color);

    int16_t pinStartX = x + 16;
    int16_t pinY = y + 8;
    for (int pin = 0; pin < 3; ++pin) {
        canvas.fillRectangle(pinStartX + (pin * 8), pinY, 5, 12, CanvasColor::White);
    }

    canvas.fillRectangle(x + 10, y + 28, 6, 8, CanvasColor::White);
}

void UiIcons::drawPowerIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    int16_t radius = 30;
    int16_t thickness = 8;

    for (int16_t offset = 0; offset < thickness; ++offset) {
        canvas.drawCircle(x, y, radius - offset, color);
    }

    canvas.fillRectangle(x - 10, y - radius - 2, 20, 20, CanvasColor::White);
    canvas.fillRoundedRectangle(x - (thickness / 2), y - radius - 4, thickness, 32, thickness / 2, color);
}

void UiIcons::drawLowBatteryIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    int16_t batteryWidth = 36;
    int16_t batteryHeight = 60;

    canvas.fillRectangle(x + (batteryWidth / 2) - 6, y - 6, 12, 6, color);
    canvas.drawRoundedRectangle(x, y, batteryWidth, batteryHeight, 4, color);
    canvas.drawRoundedRectangle(x + 1, y + 1, batteryWidth - 2, batteryHeight - 2, 3, color);

    canvas.fillRectangle(x + 2, y + batteryHeight - 12, batteryWidth - 4, 10, color);

    int16_t exclamationX = x + (batteryWidth / 2) - 3;
    int16_t exclamationY = y + 14;
    canvas.fillRoundedRectangle(exclamationX, exclamationY, 6, 20, 2, color);
    canvas.fillCircle(x + (batteryWidth / 2), y + 42, 3, color);
}

void UiIcons::drawRepeatIcon(EPaperCanvas& canvas, int16_t x, int16_t y, uint8_t repeatMode, CanvasColor color) {
    if (repeatMode == 0) {
        return;
    }

    canvas.drawHorizontalLine(x + 3, y, 7, color);
    canvas.drawHorizontalLine(x + 2, y + 7, 7, color);
    canvas.drawVerticalLine(x + 10, y + 1, 3, color);
    canvas.drawVerticalLine(x + 1, y + 4, 3, color);

    canvas.drawLine(x + 8, y - 2, x + 10, y, color);
    canvas.drawLine(x + 8, y + 2, x + 10, y, color);

    canvas.drawLine(x + 4, y + 5, x + 2, y + 7, color);
    canvas.drawLine(x + 4, y + 9, x + 2, y + 7, color);

    if (repeatMode == 2) {
        canvas.drawVerticalLine(x + 6, y + 2, 4, color);
        canvas.setPixel(x + 5, y + 3, color);
    }
}

void UiIcons::drawShuffleIcon(EPaperCanvas& canvas, int16_t x, int16_t y, bool active, CanvasColor color) {
    if (!active) {
        return;
    }

    canvas.drawLine(x, y + 1, x + 3, y + 1, color);
    canvas.drawLine(x + 3, y + 1, x + 7, y + 6, color);
    canvas.drawLine(x + 7, y + 6, x + 10, y + 6, color);

    canvas.drawLine(x, y + 6, x + 3, y + 6, color);
    canvas.drawLine(x + 3, y + 6, x + 7, y + 1, color);
    canvas.drawLine(x + 7, y + 1, x + 10, y + 1, color);

    canvas.drawLine(x + 8, y, x + 10, y + 1, color);
    canvas.drawLine(x + 8, y + 2, x + 10, y + 1, color);

    canvas.drawLine(x + 8, y + 5, x + 10, y + 6, color);
    canvas.drawLine(x + 8, y + 7, x + 10, y + 6, color);
}

void UiIcons::drawCheckmark(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color) {
    canvas.drawLine(x, y + 4, x + 3, y + 7, color);
    canvas.drawLine(x, y + 5, x + 3, y + 8, color);
    canvas.drawLine(x + 3, y + 7, x + 9, y + 1, color);
    canvas.drawLine(x + 3, y + 8, x + 9, y + 2, color);
}
