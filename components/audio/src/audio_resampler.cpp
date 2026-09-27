#include "audio_resampler.hpp"
#include <algorithm>

static uint32_t xorshift32(uint32_t& state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

AudioResampler::AudioResampler()
    : inputSampleRate(44100), outputSampleRate(44100), channels(2),
      fractionalPhase(0), phaseStep(1ULL << 32), ditherPrngState(0x12345678) {
    lastSamples[0] = 0;
    lastSamples[1] = 0;
}

void AudioResampler::configure(uint32_t sourceSampleRate, uint32_t targetSampleRate, uint8_t channelCount) {
    inputSampleRate = (sourceSampleRate > 0) ? sourceSampleRate : 44100;
    outputSampleRate = (targetSampleRate > 0) ? targetSampleRate : 44100;
    channels = (channelCount > 0 && channelCount <= 2) ? channelCount : 2;

    fractionalPhase = 0;
    phaseStep = (static_cast<uint64_t>(inputSampleRate) << 32) / outputSampleRate;
    lastSamples[0] = 0;
    lastSamples[1] = 0;
}

size_t AudioResampler::process(const int16_t* sourcePcm, size_t sourceSampleCount,
                               int16_t* destinationPcm, size_t maxDestinationSampleCount) {
    if (sourcePcm == nullptr || destinationPcm == nullptr || sourceSampleCount == 0 || maxDestinationSampleCount == 0) {
        return 0;
    }

    if (inputSampleRate == outputSampleRate) {
        size_t samplesToCopy = std::min(sourceSampleCount, maxDestinationSampleCount);
        std::copy(sourcePcm, sourcePcm + samplesToCopy, destinationPcm);
        return samplesToCopy;
    }

    size_t sourceFrames = sourceSampleCount / channels;
    size_t maxDestinationFrames = maxDestinationSampleCount / channels;
    size_t outputFrames = 0;

    while (outputFrames < maxDestinationFrames) {
        size_t inputIndex = static_cast<size_t>(fractionalPhase >> 32);
        if (inputIndex + 1 >= sourceFrames) {
            break;
        }

        uint32_t fraction = static_cast<uint32_t>(fractionalPhase & 0xFFFFFFFF);

        for (uint8_t c = 0; c < channels; ++c) {
            int32_t currentSample = sourcePcm[inputIndex * channels + c];
            int32_t nextSample = sourcePcm[(inputIndex + 1) * channels + c];

            int32_t interpolated = currentSample + static_cast<int32_t>(((nextSample - currentSample) * static_cast<int64_t>(fraction)) >> 32);
            destinationPcm[outputFrames * channels + c] = static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, interpolated)));
        }

        outputFrames++;
        fractionalPhase += phaseStep;
    }

    size_t consumedFrames = static_cast<size_t>(fractionalPhase >> 32);
    fractionalPhase &= 0xFFFFFFFF;

    if (consumedFrames > 0 && consumedFrames <= sourceFrames) {
        for (uint8_t c = 0; c < channels; ++c) {
            lastSamples[c] = sourcePcm[(sourceFrames - 1) * channels + c];
        }
    }

    return outputFrames * channels;
}

int16_t AudioResampler::applyTpdfDither24To16(int32_t sample24, uint32_t& prngState) {
    uint32_t r1 = xorshift32(prngState) & 0x1FF;
    uint32_t r2 = xorshift32(prngState) & 0x1FF;
    int32_t dither = static_cast<int32_t>(r1) - static_cast<int32_t>(r2);

    int32_t dithered = sample24 + dither;
    int32_t sample16 = dithered >> 8;

    return static_cast<int16_t>(std::max<int32_t>(-32768, std::min<int32_t>(32767, sample16)));
}
