#include "storage_manager.hpp"
#include "esp_log.h"
#include <sys/stat.h>

static const char* Tag = "StorageManager";

StorageManager::StorageManager(gpio_num_t sckPin, gpio_num_t mosiPin, gpio_num_t misoPin,
                               gpio_num_t csPin, gpio_num_t cardDetectPin)
    : pinSck(sckPin), pinMosi(mosiPin), pinMiso(misoPin), pinCs(csPin),
      pinCardDetect(cardDetectPin), cardHandle(nullptr),
      currentState(StorageState::Unmounted), stateCallback(nullptr) {
}

StorageManager::~StorageManager() {
    unmount();
}

bool StorageManager::initialize() {
    if (pinCardDetect != GPIO_NUM_NC) {
        gpio_config_t detectConfig = {};
        detectConfig.pin_bit_mask = 1ULL << pinCardDetect;
        detectConfig.mode = GPIO_MODE_INPUT;
        detectConfig.pull_up_en = GPIO_PULLUP_ENABLE;
        detectConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
        detectConfig.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&detectConfig);
    }

    if (isCardInserted()) {
        return mount();
    } else {
        currentState = StorageState::CardMissing;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
        return false;
    }
}

bool StorageManager::isCardDetectActive() const {
    if (pinCardDetect == GPIO_NUM_NC) {
        return true;
    }
    return gpio_get_level(pinCardDetect) == 0;
}

bool StorageManager::isCardInserted() const {
    return isCardDetectActive();
}

bool StorageManager::mount() {
    if (currentState == StorageState::Mounted) {
        return true;
    }

    if (!isCardInserted()) {
        currentState = StorageState::CardMissing;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
        return false;
    }

    esp_vfs_fat_sdmmc_mount_config_t mountConfig = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false
    };

    hostConfig = SDSPI_HOST_DEFAULT();
    hostConfig.slot = SPI3_HOST;

    spi_bus_config_t busConfig = {
        .mosi_io_num = pinMosi,
        .miso_io_num = pinMiso,
        .sclk_io_num = pinSck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
        .flags = 0,
        .intr_flags = 0
    };

    esp_err_t busReturn = spi_bus_initialize(SPI3_HOST, &busConfig, SDSPI_DEFAULT_DMA);
    if (busReturn != ESP_OK && busReturn != ESP_ERR_INVALID_STATE) {
        currentState = StorageState::MountFailed;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
        return false;
    }

    sdspi_device_config_t slotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotConfig.gpio_cs = pinCs;
    slotConfig.host_id = SPI3_HOST;

    esp_err_t mountReturn = esp_vfs_fat_sdspi_mount(MountPoint, &hostConfig, &slotConfig, &mountConfig, &cardHandle);
    if (mountReturn != ESP_OK) {
        currentState = StorageState::MountFailed;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
        return false;
    }

    currentState = StorageState::Mounted;
    if (stateCallback != nullptr) {
        stateCallback(currentState);
    }
    return true;
}

void StorageManager::unmount() {
    if (currentState == StorageState::Mounted && cardHandle != nullptr) {
        esp_vfs_fat_sdcard_unmount(MountPoint, cardHandle);
        cardHandle = nullptr;
        spi_bus_free(SPI3_HOST);
        currentState = StorageState::Unmounted;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
    }
}

bool StorageManager::isMounted() const {
    return currentState == StorageState::Mounted;
}

StorageState StorageManager::getState() const {
    return currentState;
}

void StorageManager::setStateCallback(StorageStateCallback callback) {
    stateCallback = callback;
}

void StorageManager::checkCardStatus() {
    bool inserted = isCardInserted();
    if (inserted && currentState != StorageState::Mounted) {
        mount();
    } else if (!inserted && currentState == StorageState::Mounted) {
        unmount();
        currentState = StorageState::CardMissing;
        if (stateCallback != nullptr) {
            stateCallback(currentState);
        }
    }
}

uint64_t StorageManager::getTotalBytes() const {
    if (cardHandle == nullptr) {
        return 0;
    }
    FATFS* filesystemHandle;
    DWORD freeClusters;
    if (f_getfree("0:", &freeClusters, &filesystemHandle) != FR_OK) {
        return 0;
    }
    uint64_t totalSectors = (filesystemHandle->n_fatent - 2) * filesystemHandle->csize;
    return totalSectors * 512;
}

uint64_t StorageManager::getFreeBytes() const {
    if (cardHandle == nullptr) {
        return 0;
    }
    FATFS* filesystemHandle;
    DWORD freeClusters;
    if (f_getfree("0:", &freeClusters, &filesystemHandle) != FR_OK) {
        return 0;
    }
    uint64_t freeSectors = freeClusters * filesystemHandle->csize;
    return freeSectors * 512;
}
