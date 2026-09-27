#include "library_screen.hpp"
#include "ui_manager.hpp"
#include "font_engine.hpp"
#include "song_list_screen.hpp"
#include <cstdio>

LibraryScreen::LibraryScreen(MusicLibrary& musicLibrary, PlaylistManager& playlistManager)
    : library(musicLibrary), playlists(playlistManager), selectedIndex(0),
      scrollOffset(0), refreshMode(EPaperDriver::RefreshMode::Full) {
    updateCategories();
}

void LibraryScreen::onEnter(UiManager& uiManager) {
    updateCategories();
    refreshMode = EPaperDriver::RefreshMode::Full;
}

void LibraryScreen::onExit() {
}

void LibraryScreen::updateCategories() {
    categories.clear();
    std::vector<std::string> userPlaylists = playlists.getPlaylistNames();
    for (const auto& playlistName : userPlaylists) {
        categories.push_back(playlistName);
    }

    if (categories.empty()) {
        categories.push_back("Favourite Songs");
        categories.push_back("DJ");
        categories.push_back("Knight Ride");
        categories.push_back("Durga Puja Slow");
        categories.push_back("Shaadi Slow");
        categories.push_back("Truck Driver");
        categories.push_back("Gym");
    }
}

void LibraryScreen::render(EPaperCanvas& canvas) {
    canvas.clear(CanvasColor::White);

    FontEngine::drawString(canvas, 8, 4, "LIBRARY", FontSize::Large, CanvasColor::Black);

    constexpr size_t ItemsPerPage = 7;
    int16_t startY = 22;
    int16_t rowHeight = 15;

    for (size_t index = 0; index < ItemsPerPage && (scrollOffset + index) < categories.size(); ++index) {
        size_t itemIndex = scrollOffset + index;
        int16_t itemY = startY + (index * rowHeight);

        char itemText[128];
        std::snprintf(itemText, sizeof(itemText), "%u. %s", static_cast<unsigned>(itemIndex + 1), categories[itemIndex].c_str());

        int16_t maxAvailableWidth = 270;
        int16_t textWidth = FontEngine::measureStringWidth(itemText, FontSize::Medium);
        if (textWidth > maxAvailableWidth) {
            textWidth = maxAvailableWidth;
        }

        if (itemIndex == selectedIndex) {
            int16_t boxWidth = (textWidth + 12 > 284) ? 284 : (textWidth + 12);
            canvas.drawRoundedRectangle(6, itemY - 2, boxWidth, rowHeight - 1, 4, CanvasColor::Black);
            FontEngine::drawStringTruncated(canvas, 10, itemY, itemText, maxAvailableWidth, FontSize::Medium, CanvasColor::Black);
        } else {
            FontEngine::drawStringTruncated(canvas, 10, itemY, itemText, maxAvailableWidth, FontSize::Medium, CanvasColor::Black);
        }
    }
}

bool LibraryScreen::handleInput(const InputEvent& event, UiManager& uiManager) {
    if (event.type == InputEventType::Button) {
        if (event.button.action == ButtonEventType::Click || event.button.action == ButtonEventType::Repeat) {
            switch (event.button.id) {
                case ButtonId::Up:
                    if (selectedIndex > 0) {
                        selectedIndex--;
                        if (selectedIndex < scrollOffset) {
                            scrollOffset = selectedIndex;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Down:
                    if (selectedIndex + 1 < categories.size()) {
                        selectedIndex++;
                        if (selectedIndex >= scrollOffset + 7) {
                            scrollOffset = selectedIndex - 6;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Enter:
                case ButtonId::EncoderSwitch:
                    if (selectedIndex < categories.size()) {
                        std::string selectedCategory = categories[selectedIndex];
                        std::vector<const TrackMetadata*> tracks;
                        if (selectedCategory == "Favourite Songs") {
                            tracks = library.getFavoriteSongs();
                            if (tracks.empty()) {
                                tracks = library.getAllSongs();
                            }
                        } else {
                            std::vector<uint32_t> ids = playlists.getTrackIds(selectedCategory);
                            for (uint32_t id : ids) {
                                const TrackMetadata* track = library.getTrackById(id);
                                if (track != nullptr) {
                                    tracks.push_back(track);
                                }
                            }
                            if (tracks.empty()) {
                                tracks = library.getAllSongs();
                            }
                        }
                        return true;
                    }
                    break;
                case ButtonId::Back:
                    uiManager.popScreen();
                    return true;
                default:
                    break;
            }
        }
    } else if (event.type == InputEventType::Encoder) {
        if (event.encoder.delta < 0 && selectedIndex > 0) {
            size_t step = std::min<size_t>(selectedIndex, static_cast<size_t>(-event.encoder.delta));
            selectedIndex -= step;
            if (selectedIndex < scrollOffset) {
                scrollOffset = selectedIndex;
            }
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        } else if (event.encoder.delta > 0 && selectedIndex + 1 < categories.size()) {
            size_t step = std::min<size_t>(categories.size() - 1 - selectedIndex, static_cast<size_t>(event.encoder.delta));
            selectedIndex += step;
            if (selectedIndex >= scrollOffset + 7) {
                scrollOffset = selectedIndex - 6;
            }
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        }
    }

    return false;
}

EPaperDriver::RefreshMode LibraryScreen::getPreferredRefreshMode() const {
    return refreshMode;
}
