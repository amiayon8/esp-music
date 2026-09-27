#include "flac_decoder.hpp"
#include <cstring>
#include <cmath>

FlacDecoder::FlacDecoder()
    : fileHandle(nullptr), sampleRate(44100), channels(2), bitDepth(16),
      durationSeconds(0), totalSamples(0), currentSamplePosition(0), audioDataOffset(0) {
}

FlacDecoder::~FlacDecoder() {
    close();
}

bool FlacDecoder::open(const std::string& filePath) {
    close();
    fileHandle = std::fopen(filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    char marker[4];
    if (std::fread(marker, 1, 4, fileHandle) != 4 || std::memcmp(marker, "fLaC", 4) != 0) {
        close();
        return false;
    }

    bool isLastBlock = false;
    while (!isLastBlock) {
        uint8_t header[4];
        if (std::fread(header, 1, 4, fileHandle) != 4) {
            break;
        }

        isLastBlock = (header[0] & 0x80) != 0;
        uint8_t blockType = header[0] & 0x7F;
        uint32_t blockLength = (static_cast<uint32_t>(header[1]) << 16) |
                               (static_cast<uint32_t>(header[2]) << 8) |
                               static_cast<uint32_t>(header[3]);

        if (blockType == 0 && blockLength >= 18) {
            uint8_t streamInfo[18];
            if (std::fread(streamInfo, 1, 18, fileHandle) == 18) {
                sampleRate = (static_cast<uint32_t>(streamInfo[10]) << 12) |
                             (static_cast<uint32_t>(streamInfo[11]) << 4) |
                             ((streamInfo[12] >> 4) & 0x0F);
                channels = ((streamInfo[12] >> 1) & 0x07) + 1;
                bitDepth = (((streamInfo[12] & 0x01) << 4) | ((streamInfo[13] >> 4) & 0x0F)) + 1;

                totalSamples = (static_cast<uint64_t>(streamInfo[13] & 0x0F) << 32) |
                               (static_cast<uint64_t>(streamInfo[14]) << 24) |
                               (static_cast<uint64_t>(streamInfo[15]) << 16) |
                               (static_cast<uint64_t>(streamInfo[16]) << 8) |
                               static_cast<uint64_t>(streamInfo[17]);
                if (sampleRate > 0) {
                    durationSeconds = static_cast<uint32_t>(totalSamples / sampleRate);
                }
            }
            if (blockLength > 18) {
                std::fseek(fileHandle, blockLength - 18, SEEK_CUR);
            }
        } else {
            std::fseek(fileHandle, blockLength, SEEK_CUR);
        }
    }

    audioDataOffset = static_cast<uint32_t>(std::ftell(fileHandle));
    currentSamplePosition = 0;
    return true;
}

DecoderStatus FlacDecoder::decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) {
    if (fileHandle == nullptr || pcmOutputBuffer == nullptr || maxSampleCount == 0) {
        samplesDecoded = 0;
        return DecoderStatus::Error;
    }

    if (currentSamplePosition >= totalSamples && totalSamples > 0) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    size_t samplesToGenerate = maxSampleCount;
    if (totalSamples > 0 && currentSamplePosition + samplesToGenerate > totalSamples) {
        samplesToGenerate = static_cast<size_t>(totalSamples - currentSamplePosition);
    }

    size_t bytesToRead = samplesToGenerate * sizeof(int16_t);
    size_t bytesRead = std::fread(pcmOutputBuffer, 1, bytesToRead, fileHandle);

    if (bytesRead == 0) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    samplesDecoded = bytesRead / sizeof(int16_t);
    currentSamplePosition += samplesDecoded / channels;

    return DecoderStatus::Success;
}

bool FlacDecoder::seek(uint32_t targetSeconds) {
    if (fileHandle == nullptr || sampleRate == 0) {
        return false;
    }

    uint64_t targetSample = static_cast<uint64_t>(targetSeconds) * sampleRate;
    if (totalSamples > 0 && targetSample > totalSamples) {
        targetSample = totalSamples;
    }

    currentSamplePosition = targetSample;
    long targetByteOffset = audioDataOffset + static_cast<long>(targetSample * channels * (bitDepth / 8));
    return std::fseek(fileHandle, targetByteOffset, SEEK_SET) == 0;
}

void FlacDecoder::close() {
    if (fileHandle != nullptr) {
        std::fclose(fileHandle);
        fileHandle = nullptr;
    }
}

uint32_t FlacDecoder::getSampleRate() const {
    return sampleRate;
}

uint8_t FlacDecoder::getChannels() const {
    return channels;
}

uint8_t FlacDecoder::getBitDepth() const {
    return bitDepth;
}

uint32_t FlacDecoder::getDurationSeconds() const {
    return durationSeconds;
}

uint32_t FlacDecoder::getCurrentPositionSeconds() const {
    if (sampleRate == 0) {
        return 0;
    }
    return static_cast<uint32_t>(currentSamplePosition / sampleRate);
}

AudioCodecType FlacDecoder::getCodecType() const {
    return AudioCodecType::FLAC;
}
