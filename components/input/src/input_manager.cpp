#include "input_manager.hpp"
#include "esp_timer.hpp"

InputManager::InputManager()
    : eventQueue(nullptr), pollTaskHandle(nullptr), isRunning(false) {
}

InputManager::~InputManager() {
    isRunning = false;
    if (pollTaskHandle != nullptr) {
        vTaskDelete(pollTaskHandle);
        pollTaskHandle = nullptr;
    }
    if (eventQueue != nullptr) {
        vQueueDelete(eventQueue);
        eventQueue = nullptr;
    }
}

bool InputManager::initialize() {
    eventQueue = xQueueCreate(32, sizeof(InputEvent));
    if (eventQueue == nullptr) {
        return false;
    }

    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Up, GPIO_NUM_32));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Down, GPIO_NUM_33));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Left, GPIO_NUM_25));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Right, GPIO_NUM_26));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Enter, GPIO_NUM_19));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Bluetooth, GPIO_NUM_2));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Library, GPIO_NUM_17));
    buttons.push_back(std::make_unique<ButtonHandler>(ButtonId::Back, GPIO_NUM_0));

    for (auto& button : buttons) {
        button->initialize();
    }

    encoder = std::make_unique<RotaryEncoder>(GPIO_NUM_36, GPIO_NUM_39);
    encoder->initialize();

    volumePotentiometer = std::make_unique<VolumePotentiometer>(ADC_UNIT_1, ADC_CHANNEL_6);
    volumePotentiometer->initialize();

    isRunning = true;
    xTaskCreatePinnedToCore(pollTaskEntry, "InputPollTask", 4096, this, 3, &pollTaskHandle, 0);
    return true;
}

void InputManager::pollTaskEntry(void* parameter) {
    InputManager* self = static_cast<InputManager*>(parameter);
    self->pollLoop();
    vTaskDelete(nullptr);
}

void InputManager::pollLoop() {
    while (isRunning) {
        uint32_t nowMs = static_cast<uint32_t>(esp_timer_get_time() / 1000);

        for (auto& button : buttons) {
            ButtonEventType action;
            if (button->update(nowMs, action)) {
                InputEvent event;
                event.type = InputEventType::Button;
                event.button.id = button->getId();
                event.button.action = action;
                xQueueSend(eventQueue, &event, 0);
            }
        }

        int32_t encoderDelta = encoder->update(nowMs);
        if (encoderDelta != 0) {
            InputEvent event;
            event.type = InputEventType::Encoder;
            event.encoder.delta = encoderDelta;
            xQueueSend(eventQueue, &event, 0);
        }

        uint8_t volumePercentage = 0;
        if (volumePotentiometer->update(volumePercentage)) {
            InputEvent event;
            event.type = InputEventType::Volume;
            event.volume.percentage = volumePercentage;
            xQueueSend(eventQueue, &event, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool InputManager::getNextEvent(InputEvent& event, uint32_t waitTimeoutMs) {
    if (eventQueue == nullptr) {
        return false;
    }
    TickType_t ticks = (waitTimeoutMs == 0) ? 0 : pdMS_TO_TICKS(waitTimeoutMs);
    return xQueueReceive(eventQueue, &event, ticks) == pdTRUE;
}
