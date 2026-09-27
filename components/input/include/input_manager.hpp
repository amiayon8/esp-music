#pragma once

#include <vector>
#include <memory>
#include "button_handler.hpp"
#include "rotary_encoder.hpp"
#include "volume_potentiometer.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

enum class InputEventType {
    Button,
    Encoder,
    Volume
};

struct InputEvent {
    InputEventType type;
    union {
        struct {
            ButtonId id;
            ButtonEventType action;
        } button;
        struct {
            int32_t delta;
        } encoder;
        struct {
            uint8_t percentage;
        } volume;
    };
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool initialize();
    bool getNextEvent(InputEvent& event, uint32_t waitTimeoutMs = 0);

private:
    std::vector<std::unique_ptr<ButtonHandler>> buttons;
    std::unique_ptr<RotaryEncoder> encoder;
    std::unique_ptr<VolumePotentiometer> volumePotentiometer;

    QueueHandle_t eventQueue;
    TaskHandle_t pollTaskHandle;
    bool isRunning;

    static void pollTaskEntry(void* parameter);
    void pollLoop();
};
