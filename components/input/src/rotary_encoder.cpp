#include "rotary_encoder.hpp"

RotaryEncoder::RotaryEncoder(gpio_num_t pinA, gpio_num_t pinB)
    : phasePinA(pinA), phasePinB(pinB), previousState(0),
      lastStepTimestampMs(0), accumulatedSteps(0) {
}

void RotaryEncoder::initialize() {
    gpio_config_t config = {};
    config.pin_bit_mask = (1ULL << phasePinA) | (1ULL << phasePinB);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);

    uint8_t a = gpio_get_level(phasePinA);
    uint8_t b = gpio_get_level(phasePinB);
    previousState = (a << 1) | b;
}

int32_t RotaryEncoder::calculateAcceleration(uint32_t deltaMs) {
    if (deltaMs < 15) {
        return 5;
    } else if (deltaMs < 30) {
        return 3;
    } else if (deltaMs < 60) {
        return 2;
    }
    return 1;
}

int32_t RotaryEncoder::update(uint32_t currentTimestampMs) {
    uint8_t a = gpio_get_level(phasePinA);
    uint8_t b = gpio_get_level(phasePinB);
    uint8_t currentState = (a << 1) | b;

    if (currentState == previousState) {
        return 0;
    }

    int8_t direction = 0;
    if ((previousState == 0b00 && currentState == 0b01) ||
        (previousState == 0b01 && currentState == 0b11) ||
        (previousState == 0b11 && currentState == 0b10) ||
        (previousState == 0b10 && currentState == 0b00)) {
        direction = 1;
    } else if ((previousState == 0b00 && currentState == 0b10) ||
               (previousState == 0b10 && currentState == 0b11) ||
               (previousState == 0b11 && currentState == 0b01) ||
               (previousState == 0b01 && currentState == 0b00)) {
        direction = -1;
    }

    previousState = currentState;

    if (direction != 0) {
        accumulatedSteps += direction;
        if (accumulatedSteps >= 4 || accumulatedSteps <= -4) {
            int32_t outputDirection = (accumulatedSteps > 0) ? 1 : -1;
            accumulatedSteps = 0;

            uint32_t deltaMs = currentTimestampMs - lastStepTimestampMs;
            lastStepTimestampMs = currentTimestampMs;

            int32_t multiplier = calculateAcceleration(deltaMs);
            return outputDirection * multiplier;
        }
    }

    return 0;
}
