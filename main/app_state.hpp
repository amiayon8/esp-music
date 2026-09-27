#pragma once

#include <memory>
#include "epaper_driver.hpp"
#include "storage_manager.hpp"
#include "music_library.hpp"
#include "playlist_manager.hpp"
#include "audio_pipeline.hpp"
#include "bluetooth_manager.hpp"
#include "playback_controller.hpp"
#include "input_manager.hpp"
#include "power_manager.hpp"
#include "ui_manager.hpp"
#include "now_playing_screen.hpp"
#include "library_screen.hpp"
#include "bluetooth_screen.hpp"
#include "error_screen.hpp"

class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();

    bool initialize();
    void run();

private:
    std::unique_ptr<EPaperDriver> displayDriver;
    std::unique_ptr<StorageManager> storage;
    std::unique_ptr<MusicLibrary> library;
    std::unique_ptr<PlaylistManager> playlists;
    std::unique_ptr<AudioPipeline> audioPipeline;
    std::unique_ptr<BluetoothManager> bluetooth;
    std::unique_ptr<PlaybackController> playback;
    std::unique_ptr<InputManager> input;
    std::unique_ptr<PowerManager> power;
    std::unique_ptr<UiManager> ui;

    std::shared_ptr<NowPlayingScreen> nowPlayingScreen;
    std::shared_ptr<LibraryScreen> libraryScreen;
    std::shared_ptr<BluetoothScreen> bluetoothScreen;
    std::shared_ptr<ErrorScreen> sdErrorScreen;
    std::shared_ptr<ErrorScreen> powerOffScreen;
    std::shared_ptr<ErrorScreen> lowBatteryScreen;

    void onStorageStateChanged(StorageState state);
    void onBluetoothStateChanged(BluetoothOverallState state);
    void onLowBattery();
};
