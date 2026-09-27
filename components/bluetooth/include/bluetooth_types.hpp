#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "esp_bt_defs.h"

enum class BluetoothOverallState {
    Off,
    On,
    Scanning,
    Connecting,
    Connected,
    Disconnected
};

enum class BluetoothCodecType {
    Sbc,
    Aac,
    Unknown
};

struct BluetoothDeviceItem {
    esp_bd_addr_t address;
    std::string name;
    int8_t rssi;
    bool isPaired;
    bool isConnected;
};
