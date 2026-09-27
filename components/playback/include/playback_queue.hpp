#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "playback_state.hpp"

class PlaybackQueue {
public:
    PlaybackQueue();

    void setQueue(const std::vector<uint32_t>& trackIds, size_t startingIndex = 0);
    void addTrackNext(uint32_t trackId);
    void addTrackToEnd(uint32_t trackId);
    void removeTrack(size_t index);
    void clear();

    bool hasCurrentTrack() const;
    uint32_t getCurrentTrackId() const;
    uint32_t getNextTrackId() const;

    bool moveToNext(RepeatMode repeatMode);
    bool moveToPrevious();

    void setShuffle(ShuffleMode mode);
    ShuffleMode getShuffle() const;

    size_t getCurrentIndex() const;
    size_t getTrackCount() const;
    std::vector<uint32_t> getUpcomingTrackIds(size_t maxCount) const;

private:
    std::vector<uint32_t> originalTrackIds;
    std::vector<size_t> playbackOrder;
    size_t currentOrderIndex;
    ShuffleMode shuffleMode;

    void rebuildPlaybackOrder();
};
