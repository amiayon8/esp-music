#include "library_index.hpp"
#include <cstdio>
#include <cstring>

static void writeString(FILE* fileHandle, const std::string& stringValue) {
    uint16_t length = static_cast<uint16_t>(stringValue.length());
    std::fwrite(&length, sizeof(uint16_t), 1, fileHandle);
    if (length > 0) {
        std::fwrite(stringValue.data(), 1, length, fileHandle);
    }
}

static std::string readString(FILE* fileHandle) {
    uint16_t length = 0;
    if (std::fread(&length, sizeof(uint16_t), 1, fileHandle) != 1 || length == 0) {
        return "";
    }
    std::vector<char> buffer(length + 1, 0);
    std::fread(buffer.data(), 1, length, fileHandle);
    return std::string(buffer.data());
}

bool LibraryIndex::saveIndex(const std::string& indexFilePath, const std::vector<TrackMetadata>& tracks) {
    FILE* fileHandle = std::fopen(indexFilePath.c_str(), "wb");
    if (fileHandle == nullptr) {
        return false;
    }

    LibraryHeader header;
    std::memcpy(header.magic, "MLIB", 4);
    header.version = 1;
    header.trackCount = static_cast<uint32_t>(tracks.size());
    header.lastScanTimestamp = 0;

    std::fwrite(&header, sizeof(LibraryHeader), 1, fileHandle);

    for (const auto& track : tracks) {
        std::fwrite(&track.id, sizeof(uint32_t), 1, fileHandle);
        writeString(fileHandle, track.filePath);
        writeString(fileHandle, track.title);
        writeString(fileHandle, track.artist);
        writeString(fileHandle, track.album);
        writeString(fileHandle, track.genre);
        std::fwrite(&track.trackNumber, sizeof(uint16_t), 1, fileHandle);
        std::fwrite(&track.discNumber, sizeof(uint16_t), 1, fileHandle);
        std::fwrite(&track.year, sizeof(uint16_t), 1, fileHandle);
        std::fwrite(&track.durationSeconds, sizeof(uint32_t), 1, fileHandle);
        uint8_t codecInt = static_cast<uint8_t>(track.codec);
        std::fwrite(&codecInt, sizeof(uint8_t), 1, fileHandle);
        std::fwrite(&track.sampleRate, sizeof(uint32_t), 1, fileHandle);
        std::fwrite(&track.bitDepth, sizeof(uint8_t), 1, fileHandle);
        std::fwrite(&track.channels, sizeof(uint8_t), 1, fileHandle);
        std::fwrite(&track.fileSize, sizeof(uint32_t), 1, fileHandle);
        writeString(fileHandle, track.artworkPath);
        uint8_t embeddedArtwork = track.hasEmbeddedArtwork ? 1 : 0;
        std::fwrite(&embeddedArtwork, sizeof(uint8_t), 1, fileHandle);
        std::fwrite(&track.embeddedArtworkOffset, sizeof(uint32_t), 1, fileHandle);
        std::fwrite(&track.embeddedArtworkSize, sizeof(uint32_t), 1, fileHandle);
        std::fwrite(&track.playCount, sizeof(uint32_t), 1, fileHandle);
        std::fwrite(&track.lastPlayedTimestamp, sizeof(uint64_t), 1, fileHandle);
        std::fwrite(&track.dateAddedTimestamp, sizeof(uint64_t), 1, fileHandle);
        uint8_t favorite = track.isFavorite ? 1 : 0;
        std::fwrite(&favorite, sizeof(uint8_t), 1, fileHandle);
    }

    std::fclose(fileHandle);
    return true;
}

bool LibraryIndex::loadIndex(const std::string& indexFilePath, std::vector<TrackMetadata>& tracks) {
    FILE* fileHandle = std::fopen(indexFilePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    LibraryHeader header;
    if (std::fread(&header, sizeof(LibraryHeader), 1, fileHandle) != 1) {
        std::fclose(fileHandle);
        return false;
    }

    if (std::memcmp(header.magic, "MLIB", 4) != 0 || header.version != 1) {
        std::fclose(fileHandle);
        return false;
    }

    tracks.clear();
    tracks.reserve(header.trackCount);

    for (uint32_t i = 0; i < header.trackCount; ++i) {
        TrackMetadata track;
        std::fread(&track.id, sizeof(uint32_t), 1, fileHandle);
        track.filePath = readString(fileHandle);
        track.title = readString(fileHandle);
        track.artist = readString(fileHandle);
        track.album = readString(fileHandle);
        track.genre = readString(fileHandle);
        std::fread(&track.trackNumber, sizeof(uint16_t), 1, fileHandle);
        std::fread(&track.discNumber, sizeof(uint16_t), 1, fileHandle);
        std::fread(&track.year, sizeof(uint16_t), 1, fileHandle);
        std::fread(&track.durationSeconds, sizeof(uint32_t), 1, fileHandle);
        uint8_t codecInt = 0;
        std::fread(&codecInt, sizeof(uint8_t), 1, fileHandle);
        track.codec = static_cast<AudioCodecType>(codecInt);
        std::fread(&track.sampleRate, sizeof(uint32_t), 1, fileHandle);
        std::fread(&track.bitDepth, sizeof(uint8_t), 1, fileHandle);
        std::fread(&track.channels, sizeof(uint8_t), 1, fileHandle);
        std::fread(&track.fileSize, sizeof(uint32_t), 1, fileHandle);
        track.artworkPath = readString(fileHandle);
        uint8_t embeddedArtwork = 0;
        std::fread(&embeddedArtwork, sizeof(uint8_t), 1, fileHandle);
        track.hasEmbeddedArtwork = (embeddedArtwork != 0);
        std::fread(&track.embeddedArtworkOffset, sizeof(uint32_t), 1, fileHandle);
        std::fread(&track.embeddedArtworkSize, sizeof(uint32_t), 1, fileHandle);
        std::fread(&track.playCount, sizeof(uint32_t), 1, fileHandle);
        std::fread(&track.lastPlayedTimestamp, sizeof(uint64_t), 1, fileHandle);
        std::fread(&track.dateAddedTimestamp, sizeof(uint64_t), 1, fileHandle);
        uint8_t favorite = 0;
        std::fread(&favorite, sizeof(uint8_t), 1, fileHandle);
        track.isFavorite = (favorite != 0);

        tracks.push_back(std::move(track));
    }

    std::fclose(fileHandle);
    return true;
}
