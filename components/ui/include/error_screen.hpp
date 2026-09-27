#pragma once

#include "screen_base.hpp"

enum class ErrorScreenType {
    SdCardNotFound,
    PowerOff,
    LowBattery
};

class ErrorScreen : public ScreenBase {
public:
    explicit ErrorScreen(ErrorScreenType type);

    void onEnter(UiManager& uiManager) override;
    void onExit() override;
    void render(EPaperCanvas& canvas) override;
    bool handleInput(const InputEvent& event, UiManager& uiManager) override;
    EPaperDriver::RefreshMode getPreferredRefreshMode() const override;

    void setType(ErrorScreenType newType);

private:
    ErrorScreenType screenType;
};
