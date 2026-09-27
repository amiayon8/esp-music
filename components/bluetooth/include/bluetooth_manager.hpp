#pragma once

#include <string>
#include <vector>
#include <functional>
#include "bluetooth_types.hpp"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class AudioPipeline;

using BluetoothStateChangedCallback = std::function<void(BluetoothOverallState)>;
using BluetoothDeviceListCallback = std::function<void(const std::vector<BluetoothDeviceItem>&)>;

class BluetoothManager {
public:
    BluetoothManager();
    ~BluetoothManager();

    bool initialize(AudioPipeline* audioPipeline);
    void deinitialize();

    void startScan();
    void stopScan();

    bool connectDevice(const esp_bd_addr_t address);
    bool disconnectDevice();
    void forgetDevice(const esp_bd_addr_t address);

    BluetoothOverallState getState() const;
    std::string getConnectedDeviceName() const;
    BluetoothCodecType getNegotiatedCodec() const;
    const char* getNegotiatedCodecName() const;
    int8_t getConnectedDeviceBattery() const;

    std::vector<BluetoothDeviceItem> getDiscoveredDevices() const;

    void setStateCallback(BluetoothStateChangedCallback callback);
    void setDeviceListCallback(BluetoothDeviceListCallback callback);

    int32_t onAudioDataRequested(uint8_t* data, int32_t length);

    void handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* parameter);
    void handleA2dpEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* parameter);
    void handleAvrcEvent(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* parameter);

private:
    AudioPipeline* pipeline;
    BluetoothOverallState currentState;
    std::string connectedDeviceName;
    esp_bd_addr_t connectedDeviceAddress;
    BluetoothCodecType negotiatedCodec;
    int8_t batteryPercentage;

    std::vector<BluetoothDeviceItem> discoveredDevices;
    mutable SemaphoreHandle_t listMutex;

    BluetoothStateChangedCallback stateCallback;
    BluetoothDeviceListCallback deviceListCallback;

    bool loadLastDeviceFromNvs(esp_bd_addr_t address, std::string& name);
    void saveLastDeviceToNvs(const esp_bd_addr_t address, const std::string& name);
    void clearLastDeviceInNvs();

    void updateDeviceInList(const esp_bd_addr_t address, const char* name, int8_t rssi);
};

extern BluetoothManager* GlobalBluetoothManagerInstance;
