#include "metadata_extractor.hpp"
#include "id3_parser.hpp"
#include "vorbis_parser.hpp"
#include "mp4_parser.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

AudioCodecType MetadataExtractor::detectCodecByExtension(const std::string& filePath) {
    size_t dotIndex = filePath.find_last_of('.');
    if (dotIndex == std::string::npos) {
        return AudioCodecType::Unknown;
    }

    std::string extension = filePath.substr(dotIndex + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "mp3") {
        return AudioCodecType::MP3;
    }
    if (extension == "flac") {
        return AudioCodecType::FLAC;
    }
    if (extension == "m4a" || extension == "aac" || extension == "mp4") {
        return AudioCodecType::AAC;
    }
    if (extension == "opus" || extension == "ogg") {
        return AudioCodecType::Opus;
    }
    if (extension == "wav") {
        return AudioCodecType::WAV;
    }
    return AudioCodecType::Unknown;
}

const char* MetadataExtractor::getCodecName(AudioCodecType codec) {
    switch (codec) {
        case AudioCodecType::MP3:
            return "MP3";
        case AudioCodecType::FLAC:
            return "FLAC";
        case AudioCodecType::AAC:
            return "AAC";
        case AudioCodecType::Opus:
            return "Opus";
        case AudioCodecType::WAV:
            return "WAV";
        default:
            return "Unknown";
    }
}

bool MetadataExtractor::extract(const std::string& filePath, TrackMetadata& metadata) {
    metadata.filePath = filePath;
    metadata.codec = detectCodecByExtension(filePath);
    metadata.hasEmbeddedArtwork = false;
    metadata.embeddedArtworkOffset = 0;
    metadata.embeddedArtworkSize = 0;
    metadata.trackNumber = 0;
    metadata.discNumber = 1;
    metadata.year = 0;
    metadata.durationSeconds = 0;
    metadata.sampleRate = 44100;
    metadata.bitDepth = 16;
    metadata.channels = 2;
    metadata.playCount = 0;
    metadata.lastPlayedTimestamp = 0;
    metadata.dateAddedTimestamp = 0;
    metadata.isFavorite = false;

    size_t lastSlash = filePath.find_last_of("/\\");
    std::string filenameWithExtension = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);
    size_t lastDot = filenameWithExtension.find_last_of('.');
    metadata.title = (lastDot == std::string::npos) ? filenameWithExtension : filenameWithExtension.substr(0, lastDot);
    metadata.artist = "Unknown Artist";
    metadata.album = "Unknown Album";
    metadata.genre = "Unknown Genre";

    FILE* fileHandle = std::fopen(filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_END);
    metadata.fileSize = static_cast<uint32_t>(std::ftell(fileHandle));
    std::fseek(fileHandle, 0, SEEK_SET);

    bool parsedSuccessfully = false;
    switch (metadata.codec) {
        case AudioCodecType::MP3:
            parsedSuccessfully = Id3Parser::parse(fileHandle, metadata);
            break;
        case AudioCodecType::FLAC:
            parsedSuccessfully = VorbisParser::parseFlac(fileHandle, metadata);
            break;
        case AudioCodecType::AAC:
            parsedSuccessfully = Mp4Parser::parse(fileHandle, metadata);
            break;
        case AudioCodecType::Opus:
            parsedSuccessfully = VorbisParser::parseOggOpus(fileHandle, metadata);
            break;
        case AudioCodecType::WAV:
            metadata.durationSeconds = metadata.fileSize / (44100 * 4);
            parsedSuccessfully = true;
            break;
        default:
            break;
    }

    std::fclose(fileHandle);
    return parsedSuccessfully;
}
