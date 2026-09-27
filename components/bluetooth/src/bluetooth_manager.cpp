#include "bluetooth_manager.hpp"
#include "audio_pipeline.hpp"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cstring>
#include <algorithm>

BluetoothManager* GlobalBluetoothManagerInstance = nullptr;

static int32_t a2dpDataCallback(uint8_t* data, int32_t length) {
    if (GlobalBluetoothManagerInstance != nullptr) {
        return GlobalBluetoothManagerInstance->onAudioDataRequested(data, length);
    }
    return 0;
}

static void gapCallbackWrapper(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
    if (GlobalBluetoothManagerInstance != nullptr) {
        GlobalBluetoothManagerInstance->handleGapEvent(event, param);
    }
}

static void a2dpCallbackWrapper(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param) {
    if (GlobalBluetoothManagerInstance != nullptr) {
        GlobalBluetoothManagerInstance->handleA2dpEvent(event, param);
    }
}

static void avrcCallbackWrapper(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* param) {
    if (GlobalBluetoothManagerInstance != nullptr) {
        GlobalBluetoothManagerInstance->handleAvrcEvent(event, param);
    }
}

BluetoothManager::BluetoothManager()
    : pipeline(nullptr), currentState(BluetoothOverallState::Off),
      negotiatedCodec(BluetoothCodecType::Sbc), batteryPercentage(-1),
      stateCallback(nullptr), deviceListCallback(nullptr) {
    listMutex = xSemaphoreCreateMutex();
    std::memset(connectedDeviceAddress, 0, sizeof(esp_bd_addr_t));
    GlobalBluetoothManagerInstance = this;
}

BluetoothManager::~BluetoothManager() {
    deinitialize();
    if (listMutex != nullptr) {
        vSemaphoreDelete(listMutex);
        listMutex = nullptr;
    }
    GlobalBluetoothManagerInstance = nullptr;
}

bool BluetoothManager::initialize(AudioPipeline* audioPipeline) {
    pipeline = audioPipeline;

    esp_err_t nvsReturn = nvs_flash_init();
    if (nvsReturn == ESP_ERR_NVS_NO_FREE_PAGES || nvsReturn == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_bt_controller_config_t btConfiguration = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&btConfiguration) != ESP_OK) {
        return false;
    }

    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
        return false;
    }

    if (esp_bluedroid_init() != ESP_OK) {
        return false;
    }

    if (esp_bluedroid_enable() != ESP_OK) {
        return false;
    }

    esp_bt_gap_register_callback(gapCallbackWrapper);
    esp_a2d_register_callback(a2dpCallbackWrapper);
    esp_a2d_source_register_data_callback(a2dpDataCallback);
    esp_a2d_source_init();

    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(avrcCallbackWrapper);

    currentState = BluetoothOverallState::Disconnected;

    esp_bd_addr_t lastAddress;
    std::string lastName;
    if (loadLastDeviceFromNvs(lastAddress, lastName)) {
        currentState = BluetoothOverallState::Connecting;
        connectedDeviceName = lastName;
        std::memcpy(connectedDeviceAddress, lastAddress, sizeof(esp_bd_addr_t));
        esp_a2d_source_connect(connectedDeviceAddress);
    }

    return true;
}

void BluetoothManager::deinitialize() {
    disconnectDevice();
    esp_avrc_ct_deinit();
    esp_a2d_source_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    currentState = BluetoothOverallState::Off;
}

void BluetoothManager::startScan() {
    if (currentState == BluetoothOverallState::Off) {
        return;
    }

    if (xSemaphoreTake(listMutex, portMAX_DELAY) == pdTRUE) {
        discoveredDevices.clear();
        xSemaphoreGive(listMutex);
    }

    currentState = BluetoothOverallState::Scanning;
    if (stateCallback != nullptr) {
        stateCallback(currentState);
    }

    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}

void BluetoothManager::stopScan() {
    esp_bt_gap_cancel_discovery();
    if (currentState == BluetoothOverallState::Scanning) {
        currentState = BluetoothOverallState::Disconnected;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
    }
}

bool BluetoothManager::connectDevice(const esp_bd_addr_t address) {
    stopScan();
    currentState = BluetoothOverallState::Connecting;
    std::memcpy(connectedDeviceAddress, address, sizeof(esp_bd_addr_t));

    if (xSemaphoreTake(listMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& device : discoveredDevices) {
            if (std::memcmp(device.address, address, sizeof(esp_bd_addr_t)) == 0) {
                connectedDeviceName = device.name;
                break;
            }
        }
        xSemaphoreGive(listMutex);
    }

    if (stateCallback != nullptr) {
        stateCallback(currentState);
    }

    return esp_a2d_source_connect(connectedDeviceAddress) == ESP_OK;
}

bool BluetoothManager::disconnectDevice() {
    if (currentState == BluetoothOverallState::Connected || currentState == BluetoothOverallState::Connecting) {
        esp_a2d_source_disconnect(connectedDeviceAddress);
        currentState = BluetoothOverallState::Disconnected;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
        return true;
    }
    return false;
}

void BluetoothManager::forgetDevice(const esp_bd_addr_t address) {
    clearLastDeviceInNvs();
    if (std::memcmp(connectedDeviceAddress, address, sizeof(esp_bd_addr_t)) == 0) {
        disconnectDevice();
    }
}

BluetoothOverallState BluetoothManager::getState() const {
    return currentState;
}

std::string BluetoothManager::getConnectedDeviceName() const {
    return connectedDeviceName;
}

BluetoothCodecType BluetoothManager::getNegotiatedCodec() const {
    return negotiatedCodec;
}

const char* BluetoothManager::getNegotiatedCodecName() const {
    switch (negotiatedCodec) {
        case BluetoothCodecType::Sbc:
            return "SBC";
        case BluetoothCodecType::Aac:
            return "AAC";
        default:
            return "Unknown";
    }
}

int8_t BluetoothManager::getConnectedDeviceBattery() const {
    return batteryPercentage;
}

std::vector<BluetoothDeviceItem> BluetoothManager::getDiscoveredDevices() const {
    std::vector<BluetoothDeviceItem> devices;
    if (xSemaphoreTake(listMutex, portMAX_DELAY) == pdTRUE) {
        devices = discoveredDevices;
        xSemaphoreGive(listMutex);
    }
    return devices;
}

void BluetoothManager::setStateCallback(BluetoothStateChangedCallback callback) {
    stateCallback = callback;
}

void BluetoothManager::setDeviceListCallback(BluetoothDeviceListCallback callback) {
    deviceListCallback = callback;
}

int32_t BluetoothManager::onAudioDataRequested(uint8_t* data, int32_t length) {
    if (pipeline == nullptr || data == nullptr || length <= 0) {
        return 0;
    }
    return static_cast<int32_t>(pipeline->pullPcmDataForBluetooth(data, static_cast<size_t>(length)));
}

void BluetoothManager::handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* parameter) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        const char* deviceName = "";
        char temporaryName[64] = {};
        for (int i = 0; i < parameter->disc_res.num_prop; ++i) {
            if (parameter->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_BDNAME) {
                uint8_t len = parameter->disc_res.prop[i].len;
                if (len >= sizeof(temporaryName)) {
                    len = sizeof(temporaryName) - 1;
                }
                std::memcpy(temporaryName, parameter->disc_res.prop[i].val, len);
                temporaryName[len] = '\0';
                deviceName = temporaryName;
            }
        }
        updateDeviceInList(parameter->disc_res.bda, deviceName, 0);
    } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
        if (parameter->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            if (currentState == BluetoothOverallState::Scanning) {
                currentState = BluetoothOverallState::Disconnected;
                if (stateCallback != nullptr) {
                    stateCallback(currentState);
                }
            }
        }
    }
}

void BluetoothManager::handleA2dpEvent(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* parameter) {
    if (event == ESP_A2D_CONNECTION_STATE_EVT) {
        if (parameter->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            currentState = BluetoothOverallState::Connected;
            std::memcpy(connectedDeviceAddress, parameter->conn_stat.remote_bda, sizeof(esp_bd_addr_t));
            saveLastDeviceToNvs(connectedDeviceAddress, connectedDeviceName);
            if (stateCallback != nullptr) {
                stateCallback(currentState);
            }
        } else if (parameter->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            currentState = BluetoothOverallState::Disconnected;
            batteryPercentage = -1;
            if (stateCallback != nullptr) {
                stateCallback(currentState);
            }
        }
    } else if (event == ESP_A2D_AUDIO_CFG_EVT) {
        if (parameter->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            negotiatedCodec = BluetoothCodecType::Sbc;
        } else if (parameter->audio_cfg.mcc.type == ESP_A2D_MCT_NON_A2DP) {
            negotiatedCodec = BluetoothCodecType::Aac;
        }
    }
}

void BluetoothManager::handleAvrcEvent(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t* parameter) {
    if (event == ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT) {
        batteryPercentage = (parameter->set_volume_rsp.volume * 100) / 127;
    }
}

void BluetoothManager::updateDeviceInList(const esp_bd_addr_t address, const char* name, int8_t rssi) {
    if (xSemaphoreTake(listMutex, portMAX_DELAY) == pdTRUE) {
        bool found = false;
        for (auto& item : discoveredDevices) {
            if (std::memcmp(item.address, address, sizeof(esp_bd_addr_t)) == 0) {
                if (name != nullptr && std::strlen(name) > 0) {
                    item.name = name;
                }
                item.rssi = rssi;
                found = true;
                break;
            }
        }
        if (!found) {
            BluetoothDeviceItem newItem;
            std::memcpy(newItem.address, address, sizeof(esp_bd_addr_t));
            newItem.name = (name != nullptr && std::strlen(name) > 0) ? name : "Audio Device";
            newItem.rssi = rssi;
            newItem.isPaired = false;
            newItem.isConnected = false;
            discoveredDevices.push_back(newItem);
        }
        xSemaphoreGive(listMutex);

        if (deviceListCallback != nullptr) {
            deviceListCallback(discoveredDevices);
        }
    }
}

bool BluetoothManager::loadLastDeviceFromNvs(esp_bd_addr_t address, std::string& name) {
    nvs_handle_t handle;
    if (nvs_open("bt_config", NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t addressSize = sizeof(esp_bd_addr_t);
    if (nvs_get_blob(handle, "last_bda", address, &addressSize) != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    size_t nameLength = 0;
    if (nvs_get_str(handle, "last_name", nullptr, &nameLength) == ESP_OK && nameLength > 0) {
        std::vector<char> buffer(nameLength);
        nvs_get_str(handle, "last_name", buffer.data(), &nameLength);
        name = buffer.data();
    } else {
        name = "Wireless Headset";
    }

    nvs_close(handle);
    return true;
}

void BluetoothManager::saveLastDeviceToNvs(const esp_bd_addr_t address, const std::string& name) {
    nvs_handle_t handle;
    if (nvs_open("bt_config", NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    nvs_set_blob(handle, "last_bda", address, sizeof(esp_bd_addr_t));
    nvs_set_str(handle, "last_name", name.c_str());
    nvs_commit(handle);
    nvs_close(handle);
}

void BluetoothManager::clearLastDeviceInNvs() {
    nvs_handle_t handle;
    if (nvs_open("bt_config", NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    nvs_erase_key(handle, "last_bda");
    nvs_erase_key(handle, "last_name");
    nvs_commit(handle);
    nvs_close(handle);
}
