#include "ui_manager.hpp"
#include "bluetooth_screen.hpp"
#include "library_screen.hpp"
#include "song_list_screen.hpp"
#include "esp_timer.h"

UiManager::UiManager(EPaperDriver& epaperDriver, InputManager& inputManager)
    : driver(epaperDriver), input(inputManager), uiTaskHandle(nullptr),
      isRunning(false), renderNeeded(true), forceFullRefresh(true),
      refreshCount(0), lastRenderTimestampMs(0), lastInputTimestampMs(0) {
}

UiManager::~UiManager() {
    isRunning = false;
    if (uiTaskHandle != nullptr) {
        vTaskDelete(uiTaskHandle);
        uiTaskHandle = nullptr;
    }
}

bool UiManager::initialize() {
    if (!canvas.allocateBuffer()) {
        return false;
    }

    if (!driver.initialize()) {
        return false;
    }

    isRunning = true;
    xTaskCreatePinnedToCore(uiTaskEntry, "UiTask", 8192, this, 2, &uiTaskHandle, 0);
    return true;
}

void UiManager::pushScreen(std::shared_ptr<ScreenBase> screen) {
    if (screen == nullptr) {
        return;
    }

    if (!screenStack.empty()) {
        screenStack.back()->onExit();
    }

    screenStack.push_back(screen);
    screen->onEnter(*this);
    requestRender(true);
}

void UiManager::popScreen() {
    if (screenStack.size() <= 1) {
        return;
    }

    screenStack.back()->onExit();
    screenStack.pop_back();

    if (!screenStack.empty()) {
        screenStack.back()->onEnter(*this);
    }
    requestRender(true);
}

void UiManager::replaceScreen(std::shared_ptr<ScreenBase> screen) {
    if (screen == nullptr) {
        return;
    }

    if (!screenStack.empty()) {
        screenStack.back()->onExit();
        screenStack.pop_back();
    }

    screenStack.push_back(screen);
    screen->onEnter(*this);
    requestRender(true);
}

void UiManager::toggleBluetoothScreen(std::shared_ptr<ScreenBase> bluetoothScreen) {
    if (screenStack.empty()) {
        pushScreen(bluetoothScreen);
        return;
    }

    if (dynamic_cast<BluetoothScreen*>(screenStack.back().get()) != nullptr) {
        popScreen();
    } else {
        pushScreen(bluetoothScreen);
    }
}

void UiManager::requestRender(bool fullRefresh) {
    renderNeeded = true;
    if (fullRefresh) {
        forceFullRefresh = true;
    }
}

void UiManager::uiTaskEntry(void* parameter) {
    UiManager* self = static_cast<UiManager*>(parameter);
    self->uiLoop();
    vTaskDelete(nullptr);
}

void UiManager::uiLoop() {
    while (isRunning) {
        uint64_t nowMs = esp_timer_get_time() / 1000;
        InputEvent event;

        while (input.getNextEvent(event, 0)) {
            lastInputTimestampMs = nowMs;

            if (event.type == InputEventType::Button && event.button.id == ButtonId::Bluetooth &&
                event.button.action == ButtonEventType::Click) {
                if (!screenStack.empty() && dynamic_cast<BluetoothScreen*>(screenStack.back().get()) != nullptr) {
                    popScreen();
                } else if (savedScreenBeforeBluetooth != nullptr) {
                    toggleBluetoothScreen(savedScreenBeforeBluetooth);
                }
                continue;
            }

            if (!screenStack.empty()) {
                if (screenStack.back()->handleInput(event, *this)) {
                    renderNeeded = true;
                }
            }
        }

        if (renderNeeded && !screenStack.empty()) {
            bool canRenderNow = forceFullRefresh || (nowMs - lastRenderTimestampMs >= 150) || (nowMs - lastInputTimestampMs >= 80);

            if (canRenderNow) {
                screenStack.back()->render(canvas);

                EPaperDriver::RefreshMode mode = screenStack.back()->getPreferredRefreshMode();
                if (forceFullRefresh || refreshCount >= 20) {
                    mode = EPaperDriver::RefreshMode::Full;
                    forceFullRefresh = false;
                    refreshCount = 0;
                } else {
                    refreshCount++;
                }

                driver.displayFrame(canvas.getBuffer(), mode);
                lastRenderTimestampMs = esp_timer_get_time() / 1000;
                renderNeeded = false;
            }
        } else if (!renderNeeded && !driver.isAsleep()) {
            if (nowMs - lastRenderTimestampMs >= 3000) {
                driver.setSleep();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
