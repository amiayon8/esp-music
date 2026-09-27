#pragma once

#include "epaper_canvas.hpp"
#include "epaper_driver.hpp"
#include "input_manager.hpp"

class UiManager;

class ScreenBase {
public:
    virtual ~ScreenBase() = default;

    virtual void onEnter(UiManager& uiManager) = 0;
    virtual void onExit() = 0;
    virtual void render(EPaperCanvas& canvas) = 0;
    virtual bool handleInput(const InputEvent& event, UiManager& uiManager) = 0;
    virtual EPaperDriver::RefreshMode getPreferredRefreshMode() const = 0;
};
