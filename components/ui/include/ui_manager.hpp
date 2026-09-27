#pragma once

#include <vector>
#include <memory>
#include "epaper_driver.hpp"
#include "epaper_canvas.hpp"
#include "screen_base.hpp"
#include "input_manager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class UiManager {
public:
    UiManager(EPaperDriver& epaperDriver, InputManager& inputManager);
    ~UiManager();

    bool initialize();

    void pushScreen(std::shared_ptr<ScreenBase> screen);
    void popScreen();
    void popToRootScreen();
    void replaceScreen(std::shared_ptr<ScreenBase> screen);

    void toggleBluetoothScreen(std::shared_ptr<ScreenBase> bluetoothScreen);
    void toggleLibraryScreen(std::shared_ptr<ScreenBase> libraryScreen);
    void setSavedScreens(std::shared_ptr<ScreenBase> libraryScreen, std::shared_ptr<ScreenBase> bluetoothScreen);

    void requestRender(bool fullRefresh = false);

private:
    EPaperDriver& driver;
    InputManager& input;
    EPaperCanvas canvas;

    std::vector<std::shared_ptr<ScreenBase>> screenStack;
    std::shared_ptr<ScreenBase> savedScreenBeforeBluetooth;
    std::shared_ptr<ScreenBase> savedScreenBeforeLibrary;

    TaskHandle_t uiTaskHandle;
    bool isRunning;
    bool renderNeeded;
    bool forceFullRefresh;
    uint32_t refreshCount;
    uint64_t lastRenderTimestampMs;
    uint64_t lastInputTimestampMs;

    static void uiTaskEntry(void* parameter);
    void uiLoop();
};
