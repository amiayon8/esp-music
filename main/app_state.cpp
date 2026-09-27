#include "app_state.hpp"
#include "hardware_pins.hpp"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

ApplicationController::ApplicationController() {
}

ApplicationController::~ApplicationController() {
}

bool ApplicationController::initialize() {
    esp_err_t nvsReturn = nvs_flash_init();
    if (nvsReturn == ESP_ERR_NVS_NO_FREE_PAGES || nvsReturn == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    displayDriver = std::make_unique<EPaperDriver>(
        HardwareConfig::DisplaySpiClock,
        HardwareConfig::DisplaySpiMosi,
        HardwareConfig::DisplayChipSelect,
        HardwareConfig::DisplayDataCommand,
        HardwareConfig::DisplayReset,
        HardwareConfig::DisplayBusy
    );

    storage = std::make_unique<StorageManager>(
        HardwareConfig::StorageSpiClock,
        HardwareConfig::StorageSpiMosi,
        HardwareConfig::StorageSpiMiso,
        HardwareConfig::StorageChipSelect,
        HardwareConfig::StorageCardDetect
    );

    storage->setStateCallback([this](StorageState state) {
        onStorageStateChanged(state);
    });

    library = std::make_unique<MusicLibrary>();
    playlists = std::make_unique<PlaylistManager>();
    playlists->initialize();

    audioPipeline = std::make_unique<AudioPipeline>();
    audioPipeline->initialize();

    bluetooth = std::make_unique<BluetoothManager>();
    bluetooth->initialize(audioPipeline.get());
    bluetooth->setStateCallback([this](BluetoothOverallState state) {
        onBluetoothStateChanged(state);
    });

    playback = std::make_unique<PlaybackController>(*audioPipeline, *library);
    playback->setTrackChangedCallback([this](const TrackMetadata* current, const TrackMetadata* next) {
        nowPlayingScreen->updateTrackArtwork(current);
        ui->requestRender(true);
    });
    playback->setPlaybackStateChangedCallback([this](AudioPipelineStatus status) {
        ui->requestRender(false);
    });

    input = std::make_unique<InputManager>();
    input->initialize();

    power = std::make_unique<PowerManager>(ADC_UNIT_1, HardwareConfig::BatteryMonitorAdcChannel);
    power->initialize();
    power->setLowBatteryCallback([this]() {
        onLowBattery();
    });

    ui = std::make_unique<UiManager>(*displayDriver, *input);
    ui->initialize();

    nowPlayingScreen = std::make_shared<NowPlayingScreen>(*playback, *bluetooth, *power);
    libraryScreen = std::make_shared<LibraryScreen>(*library, *playlists);
    bluetoothScreen = std::make_shared<BluetoothScreen>(*bluetooth);
    sdErrorScreen = std::make_shared<ErrorScreen>(ErrorScreenType::SdCardNotFound);
    powerOffScreen = std::make_shared<ErrorScreen>(ErrorScreenType::PowerOff);
    lowBatteryScreen = std::make_shared<ErrorScreen>(ErrorScreenType::LowBattery);

    storage->initialize();

    if (!storage->isMounted()) {
        ui->pushScreen(sdErrorScreen);
    } else {
        library->initialize();
        if (library->getTrackCount() == 0) {
            library->startScan(StorageManager::MusicDirectory, false);
        }
        playback->initialize();
        ui->pushScreen(nowPlayingScreen);
    }

    return true;
}

void ApplicationController::onStorageStateChanged(StorageState state) {
    if (state == StorageState::CardMissing || state == StorageState::MountFailed) {
        audioPipeline->stop();
        ui->pushScreen(sdErrorScreen);
    } else if (state == StorageState::Mounted) {
        library->initialize();
        if (library->getTrackCount() == 0) {
            library->startScan(StorageManager::MusicDirectory, false);
        }
        playback->initialize();
        ui->replaceScreen(nowPlayingScreen);
    }
}

void ApplicationController::onBluetoothStateChanged(BluetoothOverallState state) {
    ui->requestRender(false);
}

void ApplicationController::onLowBattery() {
    audioPipeline->stop();
    ui->pushScreen(lowBatteryScreen);
}

void ApplicationController::run() {
    while (true) {
        storage->checkCardStatus();
        power->update();

        if (power->hasIdleTimedOut()) {
            ui->pushScreen(powerOffScreen);
            vTaskDelay(pdMS_TO_TICKS(2000));
            power->enterDeepSleep();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
