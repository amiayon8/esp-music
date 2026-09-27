#include "song_list_screen.hpp"
#include "ui_manager.hpp"
#include "font_engine.hpp"
#include <cstdio>
#include <algorithm>

SongListScreen::SongListScreen(const std::string& title,
                               std::vector<const TrackMetadata*> songList,
                               PlaybackController& playbackController)
    : screenTitle(title), tracks(std::move(songList)), playback(playbackController),
      selectedIndex(0), pageOffset(0), refreshMode(EPaperDriver::RefreshMode::Full) {
}

void SongListScreen::onEnter(UiManager& uiManager) {
    refreshMode = EPaperDriver::RefreshMode::Full;
}

void SongListScreen::onExit() {
}

void SongListScreen::render(EPaperCanvas& canvas) {
    canvas.clear(CanvasColor::White);

    FontEngine::drawString(canvas, 8, 4, screenTitle.c_str(), FontSize::Large, CanvasColor::Black);

    constexpr size_t RowsPerColumn = 7;
    constexpr size_t ItemsPerPage = RowsPerColumn * 2;
    int16_t startY = 22;
    int16_t rowHeight = 15;
    int16_t col1X = 10;
    int16_t col2X = 152;
    int16_t maxItemWidth = 132;

    for (size_t i = 0; i < ItemsPerPage && (pageOffset + i) < tracks.size(); ++i) {
        size_t trackIndex = pageOffset + i;
        const TrackMetadata* track = tracks[trackIndex];
        if (track == nullptr) {
            continue;
        }

        size_t column = i / RowsPerColumn;
        size_t row = i % RowsPerColumn;
        int16_t itemX = (column == 0) ? col1X : col2X;
        int16_t itemY = startY + (row * rowHeight);

        char label[128];
        std::snprintf(label, sizeof(label), "%u. %s", static_cast<unsigned>(trackIndex + 1), track->title.c_str());

        if (trackIndex == selectedIndex) {
            int16_t textWidth = FontEngine::measureStringWidth(label, FontSize::Medium);
            int16_t boxWidth = std::min<int16_t>(maxItemWidth, textWidth + 8);
            canvas.drawRoundedRectangle(itemX - 4, itemY - 2, boxWidth, rowHeight - 1, 4, CanvasColor::Black);
            FontEngine::drawStringTruncated(canvas, itemX, itemY, label, maxItemWidth - 4, FontSize::Medium, CanvasColor::Black);
        } else {
            FontEngine::drawStringTruncated(canvas, itemX, itemY, label, maxItemWidth - 4, FontSize::Medium, CanvasColor::Black);
        }
    }
}

bool SongListScreen::handleInput(const InputEvent& event, UiManager& uiManager) {
    constexpr size_t RowsPerColumn = 7;
    constexpr size_t ItemsPerPage = RowsPerColumn * 2;

    if (event.type == InputEventType::Button) {
        if (event.button.action == ButtonEventType::Click || event.button.action == ButtonEventType::Repeat) {
            switch (event.button.id) {
                case ButtonId::Up:
                    if (selectedIndex > 0) {
                        selectedIndex--;
                        if (selectedIndex < pageOffset) {
                            pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Down:
                    if (selectedIndex + 1 < tracks.size()) {
                        selectedIndex++;
                        if (selectedIndex >= pageOffset + ItemsPerPage) {
                            pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Left:
                    if (selectedIndex >= RowsPerColumn) {
                        selectedIndex -= RowsPerColumn;
                        if (selectedIndex < pageOffset) {
                            pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Right:
                    if (selectedIndex + RowsPerColumn < tracks.size()) {
                        selectedIndex += RowsPerColumn;
                        if (selectedIndex >= pageOffset + ItemsPerPage) {
                            pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
                        }
                        refreshMode = EPaperDriver::RefreshMode::Partial;
                        return true;
                    }
                    break;
                case ButtonId::Enter:
                case ButtonId::EncoderSwitch:
                    if (selectedIndex < tracks.size()) {
                        std::vector<uint32_t> queueIds;
                        queueIds.reserve(tracks.size());
                        for (const auto* t : tracks) {
                            if (t != nullptr) {
                                queueIds.push_back(t->id);
                            }
                        }
                        playback.playQueue(queueIds, selectedIndex);
                        uiManager.popScreen();
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
            if (selectedIndex < pageOffset) {
                pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
            }
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        } else if (event.encoder.delta > 0 && selectedIndex + 1 < tracks.size()) {
            size_t step = std::min<size_t>(tracks.size() - 1 - selectedIndex, static_cast<size_t>(event.encoder.delta));
            selectedIndex += step;
            if (selectedIndex >= pageOffset + ItemsPerPage) {
                pageOffset = (selectedIndex / ItemsPerPage) * ItemsPerPage;
            }
            refreshMode = EPaperDriver::RefreshMode::Partial;
            return true;
        }
    }

    return false;
}

EPaperDriver::RefreshMode SongListScreen::getPreferredRefreshMode() const {
    return refreshMode;
}
