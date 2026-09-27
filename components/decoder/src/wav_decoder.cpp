#include "wav_decoder.hpp"
#include <cstring>

WavDecoder::WavDecoder()
    : fileHandle(nullptr), sampleRate(44100), channels(2), bitDepth(16),
      durationSeconds(0), dataChunkOffset(44), dataChunkSize(0), currentDataOffset(0) {
}

WavDecoder::~WavDecoder() {
    close();
}

bool WavDecoder::open(const std::string& filePath) {
    close();
    fileHandle = std::fopen(filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    uint8_t riffHeader[12];
    if (std::fread(riffHeader, 1, 12, fileHandle) != 12) {
        close();
        return false;
    }

    if (std::memcmp(riffHeader, "RIFF", 4) != 0 || std::memcmp(riffHeader + 8, "WAVE", 4) != 0) {
        close();
        return false;
    }

    while (!std::feof(fileHandle)) {
        uint8_t chunkHeader[8];
        if (std::fread(chunkHeader, 1, 8, fileHandle) != 8) {
            break;
        }

        uint32_t chunkSize = chunkHeader[4] | (chunkHeader[5] << 8) | (chunkHeader[6] << 16) | (chunkHeader[7] << 24);

        if (std::memcmp(chunkHeader, "fmt ", 4) == 0) {
            uint8_t fmtBuffer[16];
            if (std::fread(fmtBuffer, 1, 16, fileHandle) == 16) {
                channels = fmtBuffer[2] | (fmtBuffer[3] << 8);
                sampleRate = fmtBuffer[4] | (fmtBuffer[5] << 8) | (fmtBuffer[6] << 16) | (fmtBuffer[7] << 24);
                bitDepth = fmtBuffer[14] | (fmtBuffer[15] << 8);
            }
            if (chunkSize > 16) {
                std::fseek(fileHandle, chunkSize - 16, SEEK_CUR);
            }
        } else if (std::memcmp(chunkHeader, "data", 4) == 0) {
            dataChunkOffset = static_cast<uint32_t>(std::ftell(fileHandle));
            dataChunkSize = chunkSize;
            uint32_t bytesPerSecond = sampleRate * channels * (bitDepth / 8);
            if (bytesPerSecond > 0) {
                durationSeconds = dataChunkSize / bytesPerSecond;
            }
            currentDataOffset = 0;
            return true;
        } else {
            std::fseek(fileHandle, chunkSize, SEEK_CUR);
        }
    }

    return false;
}

DecoderStatus WavDecoder::decode(int16_t* pcmOutputBuffer, size_t maxSampleCount, size_t& samplesDecoded) {
    if (fileHandle == nullptr || pcmOutputBuffer == nullptr || maxSampleCount == 0) {
        samplesDecoded = 0;
        return DecoderStatus::Error;
    }

    if (currentDataOffset >= dataChunkSize) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    size_t bytesToRead = maxSampleCount * sizeof(int16_t);
    if (currentDataOffset + bytesToRead > dataChunkSize) {
        bytesToRead = dataChunkSize - currentDataOffset;
    }

    size_t bytesRead = std::fread(pcmOutputBuffer, 1, bytesToRead, fileHandle);
    if (bytesRead == 0) {
        samplesDecoded = 0;
        return DecoderStatus::EndOfFile;
    }

    samplesDecoded = bytesRead / sizeof(int16_t);
    currentDataOffset += static_cast<uint32_t>(bytesRead);

    return DecoderStatus::Success;
}

bool WavDecoder::seek(uint32_t targetSeconds) {
    if (fileHandle == nullptr || sampleRate == 0) {
        return false;
    }

    uint32_t bytesPerSecond = sampleRate * channels * (bitDepth / 8);
    uint32_t targetByte = targetSeconds * bytesPerSecond;
    if (targetByte > dataChunkSize) {
        targetByte = dataChunkSize;
    }

    currentDataOffset = targetByte;
    return std::fseek(fileHandle, dataChunkOffset + currentDataOffset, SEEK_SET) == 0;
}

void WavDecoder::close() {
    if (fileHandle != nullptr) {
        std::fclose(fileHandle);
        fileHandle = nullptr;
    }
}

uint32_t WavDecoder::getSampleRate() const {
    return sampleRate;
}

uint8_t WavDecoder::getChannels() const {
    return channels;
}

uint8_t WavDecoder::getBitDepth() const {
    return bitDepth;
}

uint32_t WavDecoder::getDurationSeconds() const {
    return durationSeconds;
}

uint32_t WavDecoder::getCurrentPositionSeconds() const {
    uint32_t bytesPerSecond = sampleRate * channels * (bitDepth / 8);
    if (bytesPerSecond == 0) {
        return 0;
    }
    return currentDataOffset / bytesPerSecond;
}

AudioCodecType WavDecoder::getCodecType() const {
    return AudioCodecType::WAV;
}
