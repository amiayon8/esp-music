#pragma once

#include <cstdint>
#include <cstddef>
#include "epaper_canvas.hpp"

class BitmapRenderer {
public:
    static void ditherGrayscaleTo1Bit(const uint8_t* grayscaleInput, uint8_t* monochromeOutput,
                                      uint16_t width, uint16_t height);
    
    static void renderArtwork(EPaperCanvas& canvas, int16_t x, int16_t y,
                              const uint8_t* monochromeBitmap, uint16_t width, uint16_t height);

    static void renderDefaultArtwork(EPaperCanvas& canvas, int16_t x, int16_t y,
                                     uint16_t size);
};
