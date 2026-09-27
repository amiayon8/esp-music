#pragma once

#include "screen_base.hpp"
#include "music_library.hpp"
#include "playlist_manager.hpp"
#include <vector>
#include <string>

class LibraryScreen : public ScreenBase {
public:
    LibraryScreen(MusicLibrary& musicLibrary, PlaylistManager& playlistManager);

    void onEnter(UiManager& uiManager) override;
    void onExit() override;
    void render(EPaperCanvas& canvas) override;
    bool handleInput(const InputEvent& event, UiManager& uiManager) override;
    EPaperDriver::RefreshMode getPreferredRefreshMode() const override;

private:
    MusicLibrary& library;
    PlaylistManager& playlists;
    std::vector<std::string> categories;
    size_t selectedIndex;
    size_t scrollOffset;
    EPaperDriver::RefreshMode refreshMode;

    void updateCategories();
};
