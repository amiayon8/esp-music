#pragma once

#include "screen_base.hpp"
#include "music_library.hpp"
#include "playback_controller.hpp"
#include <vector>
#include <string>

class SongListScreen : public ScreenBase {
public:
    SongListScreen(const std::string& title,
                   std::vector<const TrackMetadata*> songList,
                   PlaybackController& playbackController);

    void onEnter(UiManager& uiManager) override;
    void onExit() override;
    void render(EPaperCanvas& canvas) override;
    bool handleInput(const InputEvent& event, UiManager& uiManager) override;
    EPaperDriver::RefreshMode getPreferredRefreshMode() const override;

private:
    std::string screenTitle;
    std::vector<const TrackMetadata*> tracks;
    PlaybackController& playback;
    size_t selectedIndex;
    size_t pageOffset;
    EPaperDriver::RefreshMode refreshMode;
};
