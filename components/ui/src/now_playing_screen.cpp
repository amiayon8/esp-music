#include "now_playing_screen.hpp"
#include "ui_manager.hpp"
#include "font_engine.hpp"
#include "ui_icons.hpp"
#include "bitmap_renderer.hpp"
#include <cstdio>

NowPlayingScreen::NowPlayingScreen(PlaybackController& playbackController,
                                   BluetoothManager& bluetoothManager,
                                   PowerManager& powerManager)
    : playback(playbackController), bluetooth(bluetoothManager), power(powerManager),
      hasCachedArtwork(false), lastRenderedPositionSeconds(0),
      refreshMode(EPaperDriver::RefreshMode::Full) {
    cachedArtworkMonochrome.resize(ArtworkExtractor::ArtworkBufferSize, 0xFF);
}

void NowPlayingScreen::onEnter(UiManager& uiManager) {
    refreshMode = EPaperDriver::RefreshMode::Full;
    updateTrackArtwork(playback.getCurrentTrack());
}

void NowPlayingScreen::onExit() {
}

void NowPlayingScreen::updateTrackArtwork(const TrackMetadata* track) {
    if (track != nullptr) {
        hasCachedArtwork = ArtworkExtractor::loadArtworkMonochrome(*track, cachedArtworkMonochrome.data());
    } else {
        hasCachedArtwork = false;
    }
}

void NowPlayingScreen::render(EPaperCanvas& canvas) {
    canvas.clear(CanvasColor::White);

    if (hasCachedArtwork) {
        BitmapRenderer::renderArtwork(canvas, 4, 4, cachedArtworkMonochrome.data(),
                                      ArtworkExtractor::ArtworkWidth, ArtworkExtractor::ArtworkHeight);
    } else {
        BitmapRenderer::renderDefaultArtwork(canvas, 4, 4, 120);
    }

    UiIcons::drawBluetooth(canvas, 126, 6, CanvasColor::Black);

    std::string deviceName = bluetooth.getConnectedDeviceName();
    if (deviceName.empty()) {
        if (bluetooth.getState() == BluetoothOverallState::Connected) {
            deviceName = "Connected";
        } else if (bluetooth.getState() == BluetoothOverallState::Connecting) {
            deviceName = "Connecting...";
        } else if (bluetooth.getState() == BluetoothOverallState::Scanning) {
            deviceName = "Scanning...";
        } else {
            deviceName = "Disconnected";
        }
    }
    FontEngine::drawStringTruncated(canvas, 138, 6, deviceName.c_str(), 105, FontSize::Small, CanvasColor::Black);

    uint8_t batteryLevel = power.getBatteryPercentage();
    UiIcons::drawBatteryCapsule(canvas, 248, 5, batteryLevel, CanvasColor::Black);

    char batteryString[8];
    std::snprintf(batteryString, sizeof(batteryString), "%u", batteryLevel);
    FontEngine::drawString(canvas, 276, 6, batteryString, FontSize::Small, CanvasColor::Black);

    const TrackMetadata* currentTrack = playback.getCurrentTrack();
    const char* titleText = (currentTrack != nullptr && !currentTrack->title.empty()) ? currentTrack->title.c_str() : "No Track";
    const char* artistText = (currentTrack != nullptr && !currentTrack->artist.empty()) ? currentTrack->artist.c_str() : "Unknown Artist";

    FontEngine::drawStringTruncated(canvas, 126, 24, titleText, 146, FontSize::Title, CanvasColor::Black);
    FontEngine::drawStringTruncated(canvas, 126, 46, artistText, 146, FontSize::Medium, CanvasColor::Black);

    uint32_t currentSec = playback.getCurrentPositionSeconds();
    uint32_t totalSec = playback.getDurationSeconds();

    char currentBuffer[16];
    char durationBuffer[16];
    std::snprintf(currentBuffer, sizeof(currentBuffer), "%u:%02u", currentSec / 60, currentSec % 60);
    std::snprintf(durationBuffer, sizeof(durationBuffer), "%u:%02u", totalSec / 60, totalSec % 60);

    FontEngine::drawString(canvas, 126, 70, currentBuffer, FontSize::Small, CanvasColor::Black);
    FontEngine::drawString(canvas, 260, 70, durationBuffer, FontSize::Small, CanvasColor::Black);

    int16_t barStartX = 152;
    int16_t barEndX = 254;
    int16_t barWidth = barEndX - barStartX;
    canvas.drawHorizontalLine(barStartX, 73, barWidth, CanvasColor::Black);

    if (totalSec > 0) {
        int16_t progressX = barStartX + static_cast<int16_t>((currentSec * barWidth) / totalSec);
        if (progressX > barEndX) {
            progressX = barEndX;
        }
        canvas.drawHorizontalLine(barStartX, 72, progressX - barStartX + 1, CanvasColor::Black);
        canvas.drawHorizontalLine(barStartX, 73, progressX - barStartX + 1, CanvasColor::Black);
        canvas.drawHorizontalLine(barStartX, 74, progressX - barStartX + 1, CanvasColor::Black);
    }

    if (playback.getStatus() == AudioPipelineStatus::Playing) {
        UiIcons::drawPauseCircle(canvas, 134, 92, 7, CanvasColor::Black);
    } else {
        UiIcons::drawPlayCircle(canvas, 134, 92, 7, CanvasColor::Black);
    }

    UiIcons::drawRepeatIcon(canvas, 150, 88, static_cast<uint8_t>(playback.getRepeatMode()), CanvasColor::Black);
    UiIcons::drawShuffleIcon(canvas, 168, 88, playback.getShuffleMode() == ShuffleMode::On, CanvasColor::Black);

    FontEngine::drawString(canvas, 126, 104, "UP NEXT", FontSize::Small, CanvasColor::Black);

    const TrackMetadata* nextTrack = playback.getNextTrack();
    const char* upNextTitle = (nextTrack != nullptr && !nextTrack->title.empty()) ? nextTrack->title.c_str() : "--";
    FontEngine::drawStringTruncated(canvas, 126, 114, upNextTitle, 144, FontSize::Medium, CanvasColor::Black);

    UiIcons::drawSpeaker(canvas, 282, 54, CanvasColor::Black);
    uint8_t volumeLevel = playback.getVolume();
    UiIcons::drawVolumeBar(canvas, 287, 68, 38, volumeLevel, CanvasColor::Black);

    char volumeString[8];
    std::snprintf(volumeString, sizeof(volumeString), "%u", volumeLevel);
    FontEngine::drawString(canvas, 274, 114, volumeString, FontSize::Small, CanvasColor::Black);
}

bool NowPlayingScreen::handleInput(const InputEvent& event, UiManager& uiManager) {
    if (event.type == InputEventType::Button) {
        if (event.button.action == ButtonEventType::Click) {
            switch (event.button.id) {
                case ButtonId::Up:
                    playback.cycleRepeatMode();
                    refreshMode = EPaperDriver::RefreshMode::Partial;
                    return true;
                case ButtonId::Down:
                    playback.toggleShuffleMode();
                    refreshMode = EPaperDriver::RefreshMode::Partial;
                    return true;
                case ButtonId::Enter:
                case ButtonId::EncoderSwitch:
                    playback.togglePlayPause();
                    refreshMode = EPaperDriver::RefreshMode::Partial;
                    return true;
                case ButtonId::Left:
                    playback.playPrevious();
                    updateTrackArtwork(playback.getCurrentTrack());
                    refreshMode = EPaperDriver::RefreshMode::Full;
                    return true;
                case ButtonId::Right:
                    playback.playNext();
                    updateTrackArtwork(playback.getCurrentTrack());
                    refreshMode = EPaperDriver::RefreshMode::Full;
                    return true;
                case ButtonId::Back:
                    uiManager.popScreen();
                    return true;
                default:
                    break;
            }
        }
    }
 else if (event.type == InputEventType::Encoder) {
        int32_t deltaSeconds = event.encoder.delta * 5;
        playback.seek(deltaSeconds);
        refreshMode = EPaperDriver::RefreshMode::Partial;
        return true;
    } else if (event.type == InputEventType::Volume) {
        playback.setVolume(event.volume.percentage);
        refreshMode = EPaperDriver::RefreshMode::Partial;
        return true;
    }

    return false;
}

EPaperDriver::RefreshMode NowPlayingScreen::getPreferredRefreshMode() const {
    return refreshMode;
}
