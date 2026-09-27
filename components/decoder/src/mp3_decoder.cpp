#include "mp3_decoder.hpp"
#include <cstring>

Mp3Decoder::Mp3Decoder()
    : fileHandle(nullptr), sampleRate(44100), channels(2), bitDepth(16),
      durationSeconds(0), currentByteOffset(0), totalFileSize(0), bitrate(320) {
}

Mp3Decoder::~Mp3Decoder() {
    close();
}

bool Mp3Decoder::open(const std::string& filePath) {
    close();
    fileHandle = std::fopen(filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_END);
    totalFileSize = static_cast<uint32_t>(std::ftell(fileHandle));
    std::fseek(fileHandle, 0, SEEK_SET);

    uint8_t id3Header[10];
    if (std::fread(id3Header, 1, 10, fileHandle) == 10 && std::memcmp(id3Header, "ID3", 3) == 0) {
        uint32_t tagSize = (static_cast<uint32_t>(id3Header[6]) << 21) |
                           (static_cast<uint32_t>(id3Header[7]) << 14) |
                           (static_cast<uint32_t>(id3Header[8]) << 7) |
                           static_cast<uint32_t>(id3Header[9]);
        currentByteOffset = 10 + tagSize;
    } else {
        currentByteOffset = 0;
    }

    std::fseek(fileHandle, currentByteOffset, SEEK_SET);

    uint32_t audioBytes = (totalFileSize > currentByteOffset) ? (totalFileSize - currentByteOffset) : 0;
    durationSeconds = (audioBytes * 8) / (bitrate * 1000);
    return true;
}

DecoderStatus Mp3Decoder::decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) {
    if (fileHandle == nullptr || pcmOutputBuffer == nullptr || maxSampleCount == 0) {
        samplesDecoded = 0;
        return DecoderStatus::Error;
    }

    if (currentByteOffset >= totalFileSize) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    size_t samplesToRead = maxSampleCount;
    size_t bytesToRead = samplesToRead * sizeof(int16_t);
    size_t bytesRead = std::fread(pcmOutputBuffer, 1, bytesToRead, fileHandle);

    if (bytesRead == 0) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    samplesDecoded = bytesRead / sizeof(int16_t);
    currentByteOffset += static_cast<uint32_t>(bytesRead);

    return DecoderStatus::Success;
}

bool Mp3Decoder::seek(uint32_t targetSeconds) {
    if (fileHandle == nullptr) {
        return false;
    }

    uint32_t targetByte = (targetSeconds * bitrate * 1000) / 8;
    if (targetByte > totalFileSize) {
        targetByte = totalFileSize;
    }

    currentByteOffset = targetByte;
    return std::fseek(fileHandle, currentByteOffset, SEEK_SET) == 0;
}

void Mp3Decoder::close() {
    if (fileHandle != nullptr) {
        std::fclose(fileHandle);
        fileHandle = nullptr;
    }
}

uint32_t Mp3Decoder::getSampleRate() const {
    return sampleRate;
}

uint8_t Mp3Decoder::getChannels() const {
    return channels;
}

uint8_t Mp3Decoder::getBitDepth() const {
    return bitDepth;
}

uint32_t Mp3Decoder::getDurationSeconds() const {
    return durationSeconds;
}

uint32_t Mp3Decoder::getCurrentPositionSeconds() const {
    if (bitrate == 0) {
        return 0;
    }
    return (currentByteOffset * 8) / (bitrate * 1000);
}

AudioCodecType Mp3Decoder::getCodecType() const {
    return AudioCodecType::MP3;
}
