#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class AudioCodecType : uint8_t {
    Unknown = 0,
    MP3,
    FLAC,
    AAC,
    Opus,
    WAV
};

struct TrackMetadata {
    uint32_t id;
    std::string filePath;
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;
    uint16_t trackNumber;
    uint16_t discNumber;
    uint16_t year;
    uint32_t durationSeconds;
    AudioCodecType codec;
    uint32_t sampleRate;
    uint8_t bitDepth;
    uint8_t channels;
    uint32_t fileSize;
    std::string artworkPath;
    bool hasEmbeddedArtwork;
    uint32_t embeddedArtworkOffset;
    uint32_t embeddedArtworkSize;
    uint32_t playCount;
    uint64_t lastPlayedTimestamp;
    uint64_t dateAddedTimestamp;
    bool isFavorite;
};

class MetadataExtractor {
public:
    static bool extract(const std::string& filePath, TrackMetadata& metadata);
    static AudioCodecType detectCodecByExtension(const std::string& filePath);
    static const char* getCodecName(AudioCodecType codec);
};
