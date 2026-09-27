#include "opus_decoder.hpp"

OpusDecoder::OpusDecoder()
    : fileHandle(nullptr), sampleRate(48000), channels(2), bitDepth(16),
      durationSeconds(0), currentByteOffset(0), totalFileSize(0) {
}

OpusDecoder::~OpusDecoder() {
    close();
}

bool OpusDecoder::open(const std::string& filePath) {
    close();
    fileHandle = std::fopen(filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_END);
    totalFileSize = static_cast<uint32_t>(std::ftell(fileHandle));
    std::fseek(fileHandle, 0, SEEK_SET);

    currentByteOffset = 0;
    durationSeconds = (totalFileSize * 8) / (128 * 1000);
    return true;
}

DecoderStatus OpusDecoder::decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) {
    if (fileHandle == nullptr || pcmOutputBuffer == nullptr || maxSampleCount == 0) {
        samplesDecoded = 0;
        return DecoderStatus::Error;
    }

    if (currentByteOffset >= totalFileSize) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    size_t bytesToRead = maxSampleCount * sizeof(int16_t);
    size_t bytesRead = std::fread(pcmOutputBuffer, 1, bytesToRead, fileHandle);

    if (bytesRead == 0) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    samplesDecoded = bytesRead / sizeof(int16_t);
    currentByteOffset += static_cast<uint32_t>(bytesRead);

    return DecoderStatus::Success;
}

bool OpusDecoder::seek(uint32_t targetSeconds) {
    if (fileHandle == nullptr) {
        return false;
    }

    uint32_t targetByte = (targetSeconds * 128 * 1000) / 8;
    if (targetByte > totalFileSize) {
        targetByte = totalFileSize;
    }

    currentByteOffset = targetByte;
    return std::fseek(fileHandle, currentByteOffset, SEEK_SET) == 0;
}

void OpusDecoder::close() {
    if (fileHandle != nullptr) {
        std::fclose(fileHandle);
        fileHandle = nullptr;
    }
}

uint32_t OpusDecoder::getSampleRate() const {
    return sampleRate;
}

uint8_t OpusDecoder::getChannels() const {
    return channels;
}

uint8_t OpusDecoder::getBitDepth() const {
    return bitDepth;
}

uint32_t OpusDecoder::getDurationSeconds() const {
    return durationSeconds;
}

uint32_t OpusDecoder::getCurrentPositionSeconds() const {
    return (currentByteOffset * 8) / (128 * 1000);
}

AudioCodecType OpusDecoder::getCodecType() const {
    return AudioCodecType::Opus;
}
