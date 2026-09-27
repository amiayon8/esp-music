#pragma once

#include "screen_base.hpp"
#include "bluetooth_manager.hpp"
#include <vector>

class BluetoothScreen : public ScreenBase {
public:
    explicit BluetoothScreen(BluetoothManager& bluetoothManager);

    void onEnter(UiManager& uiManager) override;
    void onExit() override;
    void render(EPaperCanvas& canvas) override;
    bool handleInput(const InputEvent& event, UiManager& uiManager) override;
    EPaperDriver::RefreshMode getPreferredRefreshMode() const override;

private:
    BluetoothManager& bluetooth;
    std::vector<BluetoothDeviceItem> devices;
    size_t selectedIndex;
    EPaperDriver::RefreshMode refreshMode;

    void refreshDeviceList();
};
