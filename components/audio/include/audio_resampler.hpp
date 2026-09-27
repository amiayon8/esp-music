#pragma once

#include <cstdint>
#include <cstddef>

class AudioResampler {
public:
    AudioResampler();

    void configure(uint32_t sourceSampleRate, uint32_t targetSampleRate, uint8_t channelCount);
    size_t process(const int16_t* sourcePcm, size_t sourceSampleCount,
                   int16_t* destinationPcm, size_t maxDestinationSampleCount);

    static int16_t applyTpdfDither24To16(int32_t sample24, uint32_t& prngState);

private:
    uint32_t inputSampleRate;
    uint32_t outputSampleRate;
    uint8_t channels;
    uint64_t fractionalPhase;
    uint64_t phaseStep;
    int16_t lastSamples[2];
    uint32_t ditherPrngState;
};
