#pragma once

#include "screen_base.hpp"
#include "playback_controller.hpp"
#include "bluetooth_manager.hpp"
#include "power_manager.hpp"
#include "artwork_extractor.hpp"
#include <vector>

class NowPlayingScreen : public ScreenBase {
public:
    NowPlayingScreen(PlaybackController& playbackController,
                     BluetoothManager& bluetoothManager,
                     PowerManager& powerManager);

    void onEnter(UiManager& uiManager) override;
    void onExit() override;
    void render(EPaperCanvas& canvas) override;
    bool handleInput(const InputEvent& event, UiManager& uiManager) override;
    EPaperDriver::RefreshMode getPreferredRefreshMode() const override;

    void updateTrackArtwork(const TrackMetadata* track);

private:
    PlaybackController& playback;
    BluetoothManager& bluetooth;
    PowerManager& power;

    std::vector<uint8_t> cachedArtworkMonochrome;
    bool hasCachedArtwork;
    uint32_t lastRenderedPositionSeconds;
    EPaperDriver::RefreshMode refreshMode;
};
