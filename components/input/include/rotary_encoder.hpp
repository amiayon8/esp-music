#pragma once

#include <cstdint>
#include "driver/gpio.h"

class RotaryEncoder {
public:
    RotaryEncoder(gpio_num_t pinA, gpio_num_t pinB);

    void initialize();
    int32_t update(uint32_t currentTimestampMs);

private:
    gpio_num_t phasePinA;
    gpio_num_t phasePinB;
    uint8_t previousState;
    uint32_t lastStepTimestampMs;
    int32_t accumulatedSteps;

    int32_t calculateAcceleration(uint32_t deltaMs);
};
