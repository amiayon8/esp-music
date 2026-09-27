#include "playback_queue.hpp"
#include <algorithm>
#include <random>

PlaybackQueue::PlaybackQueue()
    : currentOrderIndex(0), shuffleMode(ShuffleMode::Off) {
}

void PlaybackQueue::setQueue(const std::vector<uint32_t>& trackIds, size_t startingIndex) {
    originalTrackIds = trackIds;
    rebuildPlaybackOrder();

    if (startingIndex < originalTrackIds.size()) {
        for (size_t i = 0; i < playbackOrder.size(); ++i) {
            if (playbackOrder[i] == startingIndex) {
                currentOrderIndex = i;
                break;
            }
        }
    } else {
        currentOrderIndex = 0;
    }
}

void PlaybackQueue::rebuildPlaybackOrder() {
    playbackOrder.clear();
    playbackOrder.reserve(originalTrackIds.size());
    for (size_t i = 0; i < originalTrackIds.size(); ++i) {
        playbackOrder.push_back(i);
    }

    if (shuffleMode == ShuffleMode::On && playbackOrder.size() > 1) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(playbackOrder.begin(), playbackOrder.end(), g);
    }
}

void PlaybackQueue::addTrackNext(uint32_t trackId) {
    originalTrackIds.push_back(trackId);
    size_t newIndex = originalTrackIds.size() - 1;
    if (currentOrderIndex + 1 < playbackOrder.size()) {
        playbackOrder.insert(playbackOrder.begin() + currentOrderIndex + 1, newIndex);
    } else {
        playbackOrder.push_back(newIndex);
    }
}

void PlaybackQueue::addTrackToEnd(uint32_t trackId) {
    originalTrackIds.push_back(trackId);
    size_t newIndex = originalTrackIds.size() - 1;
    playbackOrder.push_back(newIndex);
}

void PlaybackQueue::removeTrack(size_t index) {
    if (index >= playbackOrder.size()) {
        return;
    }
    playbackOrder.erase(playbackOrder.begin() + index);
    if (currentOrderIndex >= playbackOrder.size() && !playbackOrder.empty()) {
        currentOrderIndex = playbackOrder.size() - 1;
    }
}

void PlaybackQueue::clear() {
    originalTrackIds.clear();
    playbackOrder.clear();
    currentOrderIndex = 0;
}

bool PlaybackQueue::hasCurrentTrack() const {
    return !playbackOrder.empty() && currentOrderIndex < playbackOrder.size();
}

uint32_t PlaybackQueue::getCurrentTrackId() const {
    if (!hasCurrentTrack()) {
        return 0;
    }
    size_t originalIndex = playbackOrder[currentOrderIndex];
    return originalTrackIds[originalIndex];
}

uint32_t PlaybackQueue::getNextTrackId() const {
    if (playbackOrder.empty()) {
        return 0;
    }

    size_t nextIndex = currentOrderIndex + 1;
    if (nextIndex >= playbackOrder.size()) {
        nextIndex = 0;
    }

    size_t originalIndex = playbackOrder[nextIndex];
    return originalTrackIds[originalIndex];
}

bool PlaybackQueue::moveToNext(RepeatMode repeatMode) {
    if (playbackOrder.empty()) {
        return false;
    }

    if (repeatMode == RepeatMode::One) {
        return true;
    }

    if (currentOrderIndex + 1 < playbackOrder.size()) {
        currentOrderIndex++;
        return true;
    } else if (repeatMode == RepeatMode::All) {
        currentOrderIndex = 0;
        return true;
    }

    return false;
}

bool PlaybackQueue::moveToPrevious() {
    if (playbackOrder.empty()) {
        return false;
    }

    if (currentOrderIndex > 0) {
        currentOrderIndex--;
        return true;
    } else {
        currentOrderIndex = playbackOrder.size() - 1;
        return true;
    }
}

void PlaybackQueue::setShuffle(ShuffleMode mode) {
    if (shuffleMode == mode) {
        return;
    }
    shuffleMode = mode;
    uint32_t currentTrackId = getCurrentTrackId();
    rebuildPlaybackOrder();

    if (currentTrackId != 0) {
        for (size_t i = 0; i < playbackOrder.size(); ++i) {
            if (originalTrackIds[playbackOrder[i]] == currentTrackId) {
                currentOrderIndex = i;
                break;
            }
        }
    }
}

ShuffleMode PlaybackQueue::getShuffle() const {
    return shuffleMode;
}

size_t PlaybackQueue::getCurrentIndex() const {
    return currentOrderIndex;
}

size_t PlaybackQueue::getTrackCount() const {
    return playbackOrder.size();
}

std::vector<uint32_t> PlaybackQueue::getUpcomingTrackIds(size_t maxCount) const {
    std::vector<uint32_t> upcoming;
    for (size_t i = 1; i <= maxCount && (currentOrderIndex + i) < playbackOrder.size(); ++i) {
        size_t originalIndex = playbackOrder[currentOrderIndex + i];
        upcoming.push_back(originalTrackIds[originalIndex]);
    }
    return upcoming;
}
