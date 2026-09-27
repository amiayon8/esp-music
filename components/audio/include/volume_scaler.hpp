#pragma once

#include <cstdint>
#include <cstddef>

class VolumeScaler {
public:
    VolumeScaler();

    void setVolumePercentage(uint8_t percentage);
    uint8_t getVolumePercentage() const;
    void process(int16_t* samples, size_t sampleCount);

private:
    uint8_t currentPercentage;
    int32_t currentFixedMultiplier;
    int32_t targetFixedMultiplier;

    static int32_t calculateMultiplier(uint8_t percentage);
};
