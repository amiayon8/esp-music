#pragma once

#include <cstdint>
#include "driver/gpio.h"

enum class ButtonId {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Bluetooth,
    Library,
    Back,
    EncoderSwitch
};

enum class ButtonEventType {
    Pressed,
    Released,
    Click,
    LongPress,
    Repeat
};

struct ButtonState {
    gpio_num_t pin;
    bool isPressed;
    uint32_t pressStartTimestampMs;
    uint32_t lastRepeatTimestampMs;
    bool longPressFired;
};

class ButtonHandler {
public:
    static constexpr uint32_t DebounceDelayMs = 25;
    static constexpr uint32_t LongPressThresholdMs = 600;
    static constexpr uint32_t RepeatInitialDelayMs = 400;
    static constexpr uint32_t RepeatIntervalMs = 120;

    ButtonHandler(ButtonId id, gpio_num_t pin);

    void initialize();
    bool update(uint32_t currentTimestampMs, ButtonEventType& eventType);
    ButtonId getId() const;

private:
    ButtonId buttonId;
    ButtonState state;
    bool lastPinLevel;
    uint32_t lastDebounceTimeMs;
};
