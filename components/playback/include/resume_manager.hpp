#pragma once

#include <cstdint>
#include "playback_state.hpp"

struct ResumeData {
    uint32_t trackId;
    uint32_t positionSeconds;
    RepeatMode repeatMode;
    ShuffleMode shuffleMode;
    uint8_t volumePercentage;
};

class ResumeManager {
public:
    static constexpr uint32_t SaveIntervalSeconds = 15;

    ResumeManager();

    bool loadState(ResumeData& data);
    void saveState(const ResumeData& data, bool forceImmediate = false);

private:
    uint64_t lastSaveTimestampSeconds;
};
