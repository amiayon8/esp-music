#include "volume_scaler.hpp"
#include <algorithm>

VolumeScaler::VolumeScaler()
    : currentPercentage(100), currentFixedMultiplier(65536), targetFixedMultiplier(65536) {
}

int32_t VolumeScaler::calculateMultiplier(uint8_t percentage) {
    if (percentage == 0) {
        return 0;
    }
    if (percentage >= 100) {
        return 65536;
    }

    uint32_t p = percentage;
    uint32_t squared = (p * p * 65536) / 10000;
    return static_cast<int32_t>(squared);
}

void VolumeScaler::setVolumePercentage(uint8_t percentage) {
    currentPercentage = std::min<uint8_t>(100, percentage);
    targetFixedMultiplier = calculateMultiplier(currentPercentage);
}

uint8_t VolumeScaler::getVolumePercentage() const {
    return currentPercentage;
}

void VolumeScaler::process(int16_t* samples, size_t sampleCount) {
    if (samples == nullptr || sampleCount == 0) {
        return;
    }

    if (currentFixedMultiplier == targetFixedMultiplier && currentFixedMultiplier == 65536) {
        return;
    }

    if (currentFixedMultiplier == targetFixedMultiplier && currentFixedMultiplier == 0) {
        std::fill(samples, samples + sampleCount, 0);
        return;
    }

    int32_t step = 0;
    if (sampleCount > 0 && currentFixedMultiplier != targetFixedMultiplier) {
        step = (targetFixedMultiplier - currentFixedMultiplier) / static_cast<int32_t>(sampleCount);
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        int32_t sample = samples[i];
        int32_t scaled = (sample * currentFixedMultiplier) >> 16;
        samples[i] = static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, scaled)));

        if (step != 0) {
            currentFixedMultiplier += step;
        }
    }

    currentFixedMultiplier = targetFixedMultiplier;
}
