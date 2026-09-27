#include "vorbis_parser.hpp"
#include <vector>
#include <cstring>
#include <algorithm>

bool VorbisParser::parseFlac(FILE* fileHandle, TrackMetadata& metadata) {
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_SET);

    char marker[4];
    if (std::fread(marker, 1, 4, fileHandle) != 4 || std::memcmp(marker, "fLaC", 4) != 0) {
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
                metadata.sampleRate = (static_cast<uint32_t>(streamInfo[10]) << 12) |
                                      (static_cast<uint32_t>(streamInfo[11]) << 4) |
                                      ((streamInfo[12] >> 4) & 0x0F);
                metadata.channels = ((streamInfo[12] >> 1) & 0x07) + 1;
                metadata.bitDepth = (((streamInfo[12] & 0x01) << 4) | ((streamInfo[13] >> 4) & 0x0F)) + 1;

                uint64_t totalSamples = (static_cast<uint64_t>(streamInfo[13] & 0x0F) << 32) |
                                        (static_cast<uint64_t>(streamInfo[14]) << 24) |
                                        (static_cast<uint64_t>(streamInfo[15]) << 16) |
                                        (static_cast<uint64_t>(streamInfo[16]) << 8) |
                                        static_cast<uint64_t>(streamInfo[17]);
                if (metadata.sampleRate > 0) {
                    metadata.durationSeconds = static_cast<uint32_t>(totalSamples / metadata.sampleRate);
                }
            }
            if (blockLength > 18) {
                std::fseek(fileHandle, blockLength - 18, SEEK_CUR);
            }
        } else if (blockType == 4) {
            std::vector<uint8_t> commentData(blockLength);
            if (std::fread(commentData.data(), 1, blockLength, fileHandle) == blockLength) {
                parseVorbisCommentBlock(commentData.data(), blockLength, metadata);
            }
        } else if (blockType == 6) {
            uint32_t currentOffset = static_cast<uint32_t>(std::ftell(fileHandle));
            metadata.hasEmbeddedArtwork = true;
            metadata.embeddedArtworkOffset = currentOffset;
            metadata.embeddedArtworkSize = blockLength;
            std::fseek(fileHandle, blockLength, SEEK_CUR);
        } else {
            std::fseek(fileHandle, blockLength, SEEK_CUR);
        }
    }

    return true;
}

bool VorbisParser::parseOggOpus(FILE* fileHandle, TrackMetadata& metadata) {
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_SET);

    uint8_t buffer[256];
    size_t bytesRead = std::fread(buffer, 1, sizeof(buffer), fileHandle);
    if (bytesRead < 28 || std::memcmp(buffer, "OggS", 4) != 0) {
        return false;
    }

    metadata.sampleRate = 48000;
    metadata.bitDepth = 16;
    metadata.channels = 2;

    return true;
}

void VorbisParser::parseVorbisCommentBlock(const uint8_t* buffer, size_t length, TrackMetadata& metadata) {
    if (buffer == nullptr || length < 8) {
        return;
    }

    size_t offset = 0;
    uint32_t vendorLength = buffer[offset] | (buffer[offset + 1] << 8) |
                            (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;
    if (offset + vendorLength + 4 > length) {
        return;
    }
    offset += vendorLength;

    uint32_t userCommentListLength = buffer[offset] | (buffer[offset + 1] << 8) |
                                     (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;

    for (uint32_t i = 0; i < userCommentListLength && offset + 4 <= length; ++i) {
        uint32_t commentLength = buffer[offset] | (buffer[offset + 1] << 8) |
                                 (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
        offset += 4;
        if (offset + commentLength > length) {
            break;
        }

        std::string commentString(reinterpret_cast<const char*>(buffer + offset), commentLength);
        offset += commentLength;

        size_t equalSign = commentString.find('=');
        if (equalSign != std::string::npos) {
            std::string key = commentString.substr(0, equalSign);
            std::string value = commentString.substr(equalSign + 1);
            std::transform(key.begin(), key.end(), key.begin(), ::toupper);
            parseVorbisEntry(key, value, metadata);
        }
    }
}

void VorbisParser::parseVorbisEntry(const std::string& key, const std::string& value, TrackMetadata& metadata) {
    if (key == "TITLE") {
        metadata.title = value;
    } else if (key == "ARTIST") {
        metadata.artist = value;
    } else if (key == "ALBUM") {
        metadata.album = value;
    } else if (key == "GENRE") {
        metadata.genre = value;
    } else if (key == "TRACKNUMBER") {
        metadata.trackNumber = static_cast<uint16_t>(std::atoi(value.c_str()));
    } else if (key == "DATE" || key == "YEAR") {
        metadata.year = static_cast<uint16_t>(std::atoi(value.c_str()));
    }
}
