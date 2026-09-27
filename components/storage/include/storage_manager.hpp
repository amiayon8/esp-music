#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"

enum class StorageState {
    Unmounted,
    Mounted,
    CardMissing,
    MountFailed
};

using StorageStateCallback = std::function<void(StorageState)>;

class StorageManager {
public:
    static constexpr const char* MountPoint = "/sdcard";
    static constexpr const char* MusicDirectory = "/sdcard/Music";

    StorageManager(gpio_num_t sckPin, gpio_num_t mosiPin, gpio_num_t misoPin,
                   gpio_num_t csPin, gpio_num_t cardDetectPin);
    ~StorageManager();

    bool initialize();
    bool mount();
    void unmount();
    bool isMounted() const;
    bool isCardInserted() const;
    StorageState getState() const;
    void setStateCallback(StorageStateCallback callback);
    void checkCardStatus();

    uint64_t getTotalBytes() const;
    uint64_t getFreeBytes() const;

private:
    gpio_num_t pinSck;
    gpio_num_t pinMosi;
    gpio_num_t pinMiso;
    gpio_num_t pinCs;
    gpio_num_t pinCardDetect;

    sdmmc_card_t* cardHandle;
    sdmmc_host_t hostConfig;
    StorageState currentState;
    StorageStateCallback stateCallback;

    bool isCardDetectActive() const;
};
