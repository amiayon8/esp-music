#include "error_screen.hpp"
#include "ui_icons.hpp"
#include "font_engine.hpp"

ErrorScreen::ErrorScreen(ErrorScreenType type)
    : screenType(type) {
}

void ErrorScreen::onEnter(UiManager& uiManager) {
}

void ErrorScreen::onExit() {
}

void ErrorScreen::setType(ErrorScreenType newType) {
    screenType = newType;
}

void ErrorScreen::render(EPaperCanvas& canvas) {
    canvas.clear(CanvasColor::White);

    const char* title = "";
    const char* subtitle = "";

    switch (screenType) {
        case ErrorScreenType::SdCardNotFound:
            UiIcons::drawSdCardIcon(canvas, 120, 12, CanvasColor::Black);
            title = "SD CARD NOT FOUND";
            subtitle = "Please Insert a SD Card or a flash drive to play music";
            break;
        case ErrorScreenType::PowerOff:
            UiIcons::drawPowerIcon(canvas, 148, 44, CanvasColor::Black);
            title = "POWER OFF";
            subtitle = "Please power on to play music";
            break;
        case ErrorScreenType::LowBattery:
            UiIcons::drawLowBatteryIcon(canvas, 130, 14, CanvasColor::Black);
            title = "LOW BATTERY";
            subtitle = "Please charge the battery";
            break;
    }

    int16_t titleWidth = FontEngine::measureStringWidth(title, FontSize::Title);
    int16_t titleX = (296 - titleWidth) / 2;
    FontEngine::drawString(canvas, titleX, 86, title, FontSize::Title, CanvasColor::Black);

    int16_t subtitleWidth = FontEngine::measureStringWidth(subtitle, FontSize::Small);
    int16_t subtitleX = (296 - subtitleWidth) / 2;
    FontEngine::drawString(canvas, subtitleX, 110, subtitle, FontSize::Small, CanvasColor::Black);
}

bool ErrorScreen::handleInput(const InputEvent& event, UiManager& uiManager) {
    return false;
}

EPaperDriver::RefreshMode ErrorScreen::getPreferredRefreshMode() const {
    return EPaperDriver::RefreshMode::Full;
}
