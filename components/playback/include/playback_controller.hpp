#pragma once

#include <string>
#include <vector>
#include <functional>
#include "audio_pipeline.hpp"
#include "music_library.hpp"
#include "playback_queue.hpp"
#include "resume_manager.hpp"

using TrackChangedCallback = std::function<void(const TrackMetadata* current, const TrackMetadata* next)>;
using PlaybackStateChangedCallback = std::function<void(AudioPipelineStatus status)>;

class PlaybackController {
public:
    PlaybackController(AudioPipeline& audioPipeline, MusicLibrary& musicLibrary);

    bool initialize();

    void playTrackById(uint32_t trackId);
    void playQueue(const std::vector<uint32_t>& trackIds, size_t startingIndex = 0);
    void togglePlayPause();
    void play();
    void pause();
    void stop();
    void playNext();
    void playPrevious();
    void seek(int32_t deltaSeconds);
    void setPosition(uint32_t targetSeconds);

    void setRepeatMode(RepeatMode mode);
    RepeatMode getRepeatMode() const;
    void cycleRepeatMode();

    void setShuffleMode(ShuffleMode mode);
    ShuffleMode getShuffleMode() const;
    void toggleShuffleMode();

    void setVolume(uint8_t percentage);
    uint8_t getVolume() const;

    AudioPipelineStatus getStatus() const;
    const TrackMetadata* getCurrentTrack() const;
    const TrackMetadata* getNextTrack() const;
    uint32_t getCurrentPositionSeconds() const;
    uint32_t getDurationSeconds() const;

    void setTrackChangedCallback(TrackChangedCallback callback);
    void setPlaybackStateChangedCallback(PlaybackStateChangedCallback callback);

private:
    AudioPipeline& pipeline;
    MusicLibrary& library;
    PlaybackQueue queue;
    ResumeManager resumeManager;

    RepeatMode repeatMode;
    TrackChangedCallback trackChangedCallback;
    PlaybackStateChangedCallback playbackStateChangedCallback;

    const TrackMetadata* currentTrackMetadata;
    const TrackMetadata* nextTrackMetadata;

    void onTrackFinished();
    void loadAndStartCurrentTrack();
    void updateNextTrackPreparation();
    void persistState(bool forceImmediate);
};
