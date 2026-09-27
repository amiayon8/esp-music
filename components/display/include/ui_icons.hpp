#pragma once

#include <cstdint>
#include "epaper_canvas.hpp"

class UiIcons {
public:
    static void drawBluetooth(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);
    static void drawBatteryCapsule(EPaperCanvas& canvas, int16_t x, int16_t y, uint8_t percentage, CanvasColor color);
    static void drawSpeaker(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);
    static void drawVolumeBar(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t height, uint8_t volumePercentage, CanvasColor color);
    static void drawPlayCircle(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t radius, CanvasColor color);
    static void drawPauseCircle(EPaperCanvas& canvas, int16_t x, int16_t y, int16_t radius, CanvasColor color);
    
    static void drawSdCardIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);
    static void drawPowerIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);
    static void drawLowBatteryIcon(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);

    static void drawRepeatIcon(EPaperCanvas& canvas, int16_t x, int16_t y, uint8_t repeatMode, CanvasColor color);
    static void drawShuffleIcon(EPaperCanvas& canvas, int16_t x, int16_t y, bool active, CanvasColor color);
    static void drawCheckmark(EPaperCanvas& canvas, int16_t x, int16_t y, CanvasColor color);
};
