#include "bluetooth_screen.hpp"
#include "ui_manager.hpp"
#include "font_engine.hpp"
#include <cstdio>
#include <cstring>

BluetoothScreen::BluetoothScreen(BluetoothManager& bluetoothManager)
    : bluetooth(bluetoothManager), selectedIndex(0),
      refreshMode(EPaperDriver::RefreshMode::Full) {
}

void BluetoothScreen::onEnter(UiManager& uiManager) {
    refreshMode = EPaperDriver::RefreshMode::Full;
    refreshDeviceList();
    if (devices.empty()) {
        bluetooth.startScan();
    }
}

void BluetoothScreen::onExit() {
    bluetooth.stopScan();
}

void BluetoothScreen::refreshDeviceList() {
    devices = bluetooth.getDiscoveredDevices();
    if (devices.empty()) {
        BluetoothDeviceItem item1 = {};
        item1.name = "Stone 1208";
        devices.push_back(item1);

        BluetoothDeviceItem item2 = {};
        item2.name = "OnePlus Bullets Wireless Z2";
        devices.push_back(item2);

        BluetoothDeviceItem item3 = {};
        item3.name = "Ayon";
        devices.push_back(item3);
    }
}

void BluetoothScreen::render(EPaperCanvas& canvas) {
    canvas.clear(CanvasColor::White);

    FontEngine::drawString(canvas, 8, 4, "BLUETOOTH", FontSize::Large, CanvasColor::Black);

    const char* statusText = "";
    switch (bluetooth.getState()) {
        case BluetoothOverallState::Off:
            statusText = "OFF";
            break;
        case BluetoothOverallState::Scanning:
            statusText = "Scanning...";
            break;
        case BluetoothOverallState::Connecting:
            statusText = "Connecting...";
            break;
        case BluetoothOverallState::Connected:
            statusText = "Connected";
            break;
        case BluetoothOverallState::Disconnected:
            statusText = "Disconnected";
            break;
        default:
            break;
    }

    if (std::strlen(statusText) > 0) {
        int16_t statusWidth = FontEngine::measureStringWidth(statusText, FontSize::Small);
        FontEngine::drawString(canvas, 296 - statusWidth - 10, 8, statusText, FontSize::Small, CanvasColor::Black);
    }

    constexpr size_t ItemsPerPage = 6;
    int16_t startY = 24;
    int16_t rowHeight = 16;

    for (size_t index = 0; index < ItemsPerPage && index < devices.size(); ++index) {
        int16_t itemY = startY + (index * rowHeight);

        char label[128];
        std::snprintf(label, sizeof(label), "%u. %s", static_cast<unsigned>(index + 1), devices[index].name.c_str());

        bool isDeviceConnected = (bluetooth.getState() == BluetoothOverallState::Connected &&
                                  (devices[index].name == bluetooth.getConnectedDeviceName() || devices[index].isConnected));

        int16_t maxAvailableWidth = isDeviceConnected ? 245 : 265;
        int16_t textWidth = FontEngine::measureStringWidth(label, FontSize::Medium);
        if (textWidth > maxAvailableWidth) {
            textWidth = maxAvailableWidth;
        }

        int16_t boxWidth = isDeviceConnected ? (textWidth + 24) : (textWidth + 12);
        if (boxWidth > 284) {
            boxWidth = 284;
        }

        if (index == selectedIndex) {
            canvas.drawRoundedRectangle(6, itemY - 2, boxWidth, rowHeight - 1, 4, CanvasColor::Black);
        }

        FontEngine::drawStringTruncated(canvas, 10, itemY, label, maxAvailableWidth, FontSize::Medium, CanvasColor::Black);

        if (isDeviceConnected) {
            UiIcons::drawCheckmark(canvas, 10 + textWidth + 6, itemY + 1, CanvasColor::Black);
        }
    }
}

bool BluetoothScreen::handleInput(const InputEvent& event, UiManager& uiManager) {
    if (event.type == InputEventType::Button) {
        if (event.button.action == ButtonEventType::Click || event.button.action == ButtonEventType::Repeat) {
            switch (event.button.id) {
                case ButtonId::Up:
                    if (selectedIndex > 0) {
                        selectedIndex--;
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Down:
                    if (selectedIndex + 1 < devices.size()) {
                        selectedIndex++;
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Enter:
                case ButtonId::EncoderSwitch:
                    if (selectedIndex < devices.size()) {
                        if (bluetooth.getState() == BluetoothOverallState::Connected) {
                            bluetooth.disconnectDevice();
                        } else {
                            bluetooth.connectDevice(devices[selectedIndex].address);
                        }
                        refreshMode = EPaperDriver::RefreshMode::Full;
                        return true;
                    }
                    break;
                case ButtonId::Back:
                    uiManager.popScreen();
                    return true;
                default:
                    break;
            }
        }
    } else if (event.type == InputEventType::Encoder) {
        if (event.encoder.delta < 0 && selectedIndex > 0) {
            selectedIndex--;
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        } else if (event.encoder.delta > 0 && selectedIndex + 1 < devices.size()) {
            selectedIndex++;
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        }
    }

    return false;
}

EPaperDriver::RefreshMode BluetoothScreen::getPreferredRefreshMode() const {
    return refreshMode;
}
