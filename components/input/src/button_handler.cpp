#include "button_handler.hpp"

ButtonHandler::ButtonHandler(ButtonId id, gpio_num_t pin)
    : buttonId(id), lastPinLevel(true), lastDebounceTimeMs(0) {
    state.pin = pin;
    state.isPressed = false;
    state.pressStartTimestampMs = 0;
    state.lastRepeatTimestampMs = 0;
    state.longPressFired = false;
}

void ButtonHandler::initialize() {
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << state.pin;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
}

bool ButtonHandler::update(uint32_t currentTimestampMs, ButtonEventType& eventType) {
    bool rawPinLevel = gpio_get_level(state.pin) == 0;

    if (rawPinLevel != lastPinLevel) {
        lastDebounceTimeMs = currentTimestampMs;
        lastPinLevel = rawPinLevel;
    }

    if ((currentTimestampMs - lastDebounceTimeMs) >= DebounceDelayMs) {
        if (rawPinLevel && !state.isPressed) {
            state.isPressed = true;
            state.pressStartTimestampMs = currentTimestampMs;
            state.lastRepeatTimestampMs = currentTimestampMs;
            state.longPressFired = false;
            eventType = ButtonEventType::Pressed;
            return true;
        } else if (!rawPinLevel && state.isPressed) {
            state.isPressed = false;
            uint32_t duration = currentTimestampMs - state.pressStartTimestampMs;
            if (!state.longPressFired && duration < LongPressThresholdMs) {
                eventType = ButtonEventType::Click;
                return true;
            } else {
                eventType = ButtonEventType::Released;
                return true;
            }
        }
    }

    if (state.isPressed) {
        uint32_t heldDuration = currentTimestampMs - state.pressStartTimestampMs;

        if (!state.longPressFired && heldDuration >= LongPressThresholdMs) {
            state.longPressFired = true;
            eventType = ButtonEventType::LongPress;
            return true;
        }

        if (heldDuration >= RepeatInitialDelayMs) {
            if ((currentTimestampMs - state.lastRepeatTimestampMs) >= RepeatIntervalMs) {
                state.lastRepeatTimestampMs = currentTimestampMs;
                eventType = ButtonEventType::Repeat;
                return true;
            }
        }
    }

    return false;
}

ButtonId ButtonHandler::getId() const {
    return buttonId;
}
