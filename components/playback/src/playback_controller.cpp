#include "playback_controller.hpp"

PlaybackController::PlaybackController(AudioPipeline& audioPipeline, MusicLibrary& musicLibrary)
    : pipeline(audioPipeline), library(musicLibrary),
      repeatMode(RepeatMode::Off), trackChangedCallback(nullptr),
      playbackStateChangedCallback(nullptr), currentTrackMetadata(nullptr),
      nextTrackMetadata(nullptr) {
}

bool PlaybackController::initialize() {
    pipeline.setTrackFinishedCallback([this]() {
        onTrackFinished();
    });

    pipeline.setPositionCallback([this](uint32_t currentSeconds, uint32_t totalSeconds) {
        persistState(false);
    });

    ResumeData data;
    if (resumeManager.loadState(data)) {
        repeatMode = data.repeatMode;
        queue.setShuffle(data.shuffleMode);
        pipeline.setVolume(data.volumePercentage);

        const TrackMetadata* track = library.getTrackById(data.trackId);
        if (track != nullptr) {
            std::vector<uint32_t> singleTrack = { data.trackId };
            queue.setQueue(singleTrack, 0);
            currentTrackMetadata = track;
            pipeline.playTrack(track->filePath, track->codec);
            pipeline.seek(data.positionSeconds);
            pipeline.pause();

            if (playbackStateChangedCallback != nullptr) {
                playbackStateChangedCallback(AudioPipelineStatus::Paused);
            }
        }
        return true;
    }

    return false;
}

void PlaybackController::playTrackById(uint32_t trackId) {
    std::vector<uint32_t> single = { trackId };
    playQueue(single, 0);
}

void PlaybackController::playQueue(const std::vector<uint32_t>& trackIds, size_t startingIndex) {
    queue.setQueue(trackIds, startingIndex);
    loadAndStartCurrentTrack();
}

void PlaybackController::loadAndStartCurrentTrack() {
    uint32_t trackId = queue.getCurrentTrackId();
    if (trackId == 0) {
        pipeline.stop();
        currentTrackMetadata = nullptr;
        nextTrackMetadata = nullptr;
        if (trackChangedCallback != nullptr) {
            trackChangedCallback(nullptr, nullptr);
        }
        return;
    }

    currentTrackMetadata = library.getTrackById(trackId);
    if (currentTrackMetadata == nullptr) {
        return;
    }

    library.recordTrackPlayed(trackId);
    pipeline.playTrack(currentTrackMetadata->filePath, currentTrackMetadata->codec);

    updateNextTrackPreparation();

    if (trackChangedCallback != nullptr) {
        trackChangedCallback(currentTrackMetadata, nextTrackMetadata);
    }
    if (playbackStateChangedCallback != nullptr) {
        playbackStateChangedCallback(AudioPipelineStatus::Playing);
    }

    persistState(true);
}

void PlaybackController::updateNextTrackPreparation() {
    uint32_t nextId = queue.getNextTrackId();
    if (nextId != 0) {
        nextTrackMetadata = library.getTrackById(nextId);
        if (nextTrackMetadata != nullptr) {
            pipeline.prepareNextTrack(nextTrackMetadata->filePath, nextTrackMetadata->codec);
        }
    } else {
        nextTrackMetadata = nullptr;
    }
}

void PlaybackController::onTrackFinished() {
    if (queue.moveToNext(repeatMode)) {
        loadAndStartCurrentTrack();
    } else {
        pipeline.stop();
        if (playbackStateChangedCallback != nullptr) {
            playbackStateChangedCallback(AudioPipelineStatus::Stopped);
        }
    }
}

void PlaybackController::togglePlayPause() {
    if (pipeline.getStatus() == AudioPipelineStatus::Playing) {
        pause();
    } else if (pipeline.getStatus() == AudioPipelineStatus::Paused) {
        play();
    } else if (currentTrackMetadata != nullptr) {
        play();
    }
}

void PlaybackController::play() {
    pipeline.resume();
    if (playbackStateChangedCallback != nullptr) {
        playbackStateChangedCallback(pipeline.getStatus());
    }
}

void PlaybackController::pause() {
    pipeline.pause();
    persistState(true);
    if (playbackStateChangedCallback != nullptr) {
        playbackStateChangedCallback(pipeline.getStatus());
    }
}

void PlaybackController::stop() {
    pipeline.stop();
    persistState(true);
    if (playbackStateChangedCallback != nullptr) {
        playbackStateChangedCallback(pipeline.getStatus());
    }
}

void PlaybackController::playNext() {
    if (queue.moveToNext(repeatMode)) {
        loadAndStartCurrentTrack();
    }
}

void PlaybackController::playPrevious() {
    if (pipeline.getCurrentPositionSeconds() > 3) {
        pipeline.seek(0);
    } else {
        if (queue.moveToPrevious()) {
            loadAndStartCurrentTrack();
        }
    }
}

void PlaybackController::seek(int32_t deltaSeconds) {
    int32_t currentPos = static_cast<int32_t>(pipeline.getCurrentPositionSeconds());
    int32_t newPos = currentPos + deltaSeconds;
    if (newPos < 0) {
        newPos = 0;
    }
    pipeline.seek(static_cast<uint32_t>(newPos));
}

void PlaybackController::setPosition(uint32_t targetSeconds) {
    pipeline.seek(targetSeconds);
}

void PlaybackController::setRepeatMode(RepeatMode mode) {
    repeatMode = mode;
}

RepeatMode PlaybackController::getRepeatMode() const {
    return repeatMode;
}

void PlaybackController::cycleRepeatMode() {
    if (repeatMode == RepeatMode::Off) {
        repeatMode = RepeatMode::All;
    } else if (repeatMode == RepeatMode::All) {
        repeatMode = RepeatMode::One;
    } else {
        repeatMode = RepeatMode::Off;
    }
}

void PlaybackController::setShuffleMode(ShuffleMode mode) {
    queue.setShuffle(mode);
    updateNextTrackPreparation();
    if (trackChangedCallback != nullptr) {
        trackChangedCallback(currentTrackMetadata, nextTrackMetadata);
    }
}

ShuffleMode PlaybackController::getShuffleMode() const {
    return queue.getShuffle();
}

void PlaybackController::toggleShuffleMode() {
    setShuffleMode(queue.getShuffle() == ShuffleMode::On ? ShuffleMode::Off : ShuffleMode::On);
}

void PlaybackController::setVolume(uint8_t percentage) {
    pipeline.setVolume(percentage);
    persistState(false);
}

uint8_t PlaybackController::getVolume() const {
    return pipeline.getVolume();
}

AudioPipelineStatus PlaybackController::getStatus() const {
    return pipeline.getStatus();
}

const TrackMetadata* PlaybackController::getCurrentTrack() const {
    return currentTrackMetadata;
}

const TrackMetadata* PlaybackController::getNextTrack() const {
    return nextTrackMetadata;
}

uint32_t PlaybackController::getCurrentPositionSeconds() const {
    return pipeline.getCurrentPositionSeconds();
}

uint32_t PlaybackController::getDurationSeconds() const {
    return pipeline.getDurationSeconds();
}

void PlaybackController::setTrackChangedCallback(TrackChangedCallback callback) {
    trackChangedCallback = callback;
}

void PlaybackController::setPlaybackStateChangedCallback(PlaybackStateChangedCallback callback) {
    playbackStateChangedCallback = callback;
}

void PlaybackController::persistState(bool forceImmediate) {
    if (currentTrackMetadata == nullptr) {
        return;
    }
    ResumeData data;
    data.trackId = currentTrackMetadata->id;
    data.positionSeconds = pipeline.getCurrentPositionSeconds();
    data.repeatMode = repeatMode;
    data.shuffleMode = queue.getShuffle();
    data.volumePercentage = pipeline.getVolume();
    resumeManager.saveState(data, forceImmediate);
}
