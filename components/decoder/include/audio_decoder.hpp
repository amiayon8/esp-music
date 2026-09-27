#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include "metadata_extractor.hpp"

enum class DecoderStatus {
    Success,
    EndOfFile,
    Error,
    BufferUnderflow
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    virtual bool open(const std::string& filePath) = 0;
    virtual DecoderStatus decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) = 0;
    virtual bool seek(uint32_t targetSeconds) = 0;
    virtual void close() = 0;

    virtual uint32_t getSampleRate() const = 0;
    virtual uint8_t getChannels() const = 0;
    virtual uint8_t getBitDepth() const = 0;
    virtual uint32_t getDurationSeconds() const = 0;
    virtual uint32_t getCurrentPositionSeconds() const = 0;
    virtual AudioCodecType getCodecType() const = 0;

    static std::unique_ptr<AudioDecoder> createDecoder(AudioCodecType codec);
};
