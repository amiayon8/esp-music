#pragma once

#include "audio_decoder.hpp"
#include <cstdio>
#include <vector>

class FlacDecoder : public AudioDecoder {
public:
    FlacDecoder();
    ~FlacDecoder() override;

    bool open(const std::string& filePath) override;
    DecoderStatus decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) override;
    bool seek(uint32_t targetSeconds) override;
    void close() override;

    uint32_t getSampleRate() const override;
    uint8_t getChannels() const override;
    uint8_t getBitDepth() const override;
    uint32_t getDurationSeconds() const override;
    uint32_t getCurrentPositionSeconds() const override;
    AudioCodecType getCodecType() const override;

private:
    FILE* fileHandle;
    uint32_t sampleRate;
    uint8_t channels;
    uint8_t bitDepth;
    uint32_t durationSeconds;
    uint64_t totalSamples;
    uint64_t currentSamplePosition;
    uint32_t audioDataOffset;
};
