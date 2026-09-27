#include "id3_parser.hpp"
#include <cstring>
#include <cstdlib>

uint32_t Id3Parser::decodeSynchsafeInteger(const uint8_t* bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 21) |
           (static_cast<uint32_t>(bytes[1]) << 14) |
           (static_cast<uint32_t>(bytes[2]) << 7) |
           static_cast<uint32_t>(bytes[3]);
}

bool Id3Parser::parse(FILE* fileHandle, TrackMetadata& metadata) {
    if (fileHandle == nullptr) {
        return false;
    }

    bool id3v2Parsed = parseId3v2(fileHandle, metadata);
    if (!id3v2Parsed || metadata.title.empty() || metadata.artist == "Unknown Artist") {
        parseId3v1(fileHandle, metadata);
    }
    return true;
}

bool Id3Parser::parseId3v1(FILE* fileHandle, TrackMetadata& metadata) {
    if (std::fseek(fileHandle, -128, SEEK_END) != 0) {
        return false;
    }

    uint8_t buffer[128];
    if (std::fread(buffer, 1, 128, fileHandle) != 128) {
        return false;
    }

    if (std::memcmp(buffer, "TAG", 3) != 0) {
        return false;
    }

    char titleBuffer[31] = {};
    char artistBuffer[31] = {};
    char albumBuffer[31] = {};

    std::memcpy(titleBuffer, buffer + 3, 30);
    std::memcpy(artistBuffer, buffer + 33, 30);
    std::memcpy(albumBuffer, buffer + 63, 30);

    for (int index = 29; index >= 0 && titleBuffer[index] == ' '; --index) {
        titleBuffer[index] = '\0';
    }
    for (int index = 29; index >= 0 && artistBuffer[index] == ' '; --index) {
        artistBuffer[index] = '\0';
    }
    for (int index = 29; index >= 0 && albumBuffer[index] == ' '; --index) {
        albumBuffer[index] = '\0';
    }

    if (std::strlen(titleBuffer) > 0) {
        metadata.title = titleBuffer;
    }
    if (std::strlen(artistBuffer) > 0) {
        metadata.artist = artistBuffer;
    }
    if (std::strlen(albumBuffer) > 0) {
        metadata.album = albumBuffer;
    }

    if (buffer[125] == 0 && buffer[126] != 0) {
        metadata.trackNumber = buffer[126];
    }
    return true;
}

bool Id3Parser::parseId3v2(FILE* fileHandle, TrackMetadata& metadata) {
    std::fseek(fileHandle, 0, SEEK_SET);

    uint8_t header[10];
    if (std::fread(header, 1, 10, fileHandle) != 10) {
        return false;
    }

    if (std::memcmp(header, "ID3", 3) != 0) {
        return false;
    }

    uint8_t majorVersion = header[3];
    uint32_t tagSize = decodeSynchsafeInteger(header + 6);
    uint32_t bytesRead = 0;

    while (bytesRead + 10 <= tagSize) {
        uint8_t frameHeader[10];
        if (std::fread(frameHeader, 1, 10, fileHandle) != 10) {
            break;
        }
        bytesRead += 10;

        if (frameHeader[0] == 0) {
            break;
        }

        char frameId[5] = {
            static_cast<char>(frameHeader[0]),
            static_cast<char>(frameHeader[1]),
            static_cast<char>(frameHeader[2]),
            static_cast<char>(frameHeader[3]),
            '\0'
        };

        uint32_t frameSize = (majorVersion == 4) ?
            decodeSynchsafeInteger(frameHeader + 4) :
            (static_cast<uint32_t>(frameHeader[4]) << 24) |
            (static_cast<uint32_t>(frameHeader[5]) << 16) |
            (static_cast<uint32_t>(frameHeader[6]) << 8) |
            static_cast<uint32_t>(frameHeader[7]);

        if (frameSize == 0 || bytesRead + frameSize > tagSize + 10) {
            break;
        }

        if (std::strcmp(frameId, "TIT2") == 0) {
            std::vector<char> content(frameSize + 1, 0);
            std::fread(content.data(), 1, frameSize, fileHandle);
            size_t textOffset = (content[0] == 0) ? 1 : 2;
            if (textOffset < content.size()) {
                metadata.title = std::string(content.data() + textOffset);
            }
        } else if (std::strcmp(frameId, "TPE1") == 0) {
            std::vector<char> content(frameSize + 1, 0);
            std::fread(content.data(), 1, frameSize, fileHandle);
            size_t textOffset = (content[0] == 0) ? 1 : 2;
            if (textOffset < content.size()) {
                metadata.artist = std::string(content.data() + textOffset);
            }
        } else if (std::strcmp(frameId, "TALB") == 0) {
            std::vector<char> content(frameSize + 1, 0);
            std::fread(content.data(), 1, frameSize, fileHandle);
            size_t textOffset = (content[0] == 0) ? 1 : 2;
            if (textOffset < content.size()) {
                metadata.album = std::string(content.data() + textOffset);
            }
        } else if (std::strcmp(frameId, "TCON") == 0) {
            std::vector<char> content(frameSize + 1, 0);
            std::fread(content.data(), 1, frameSize, fileHandle);
            size_t textOffset = (content[0] == 0) ? 1 : 2;
            if (textOffset < content.size()) {
                metadata.genre = std::string(content.data() + textOffset);
            }
        } else if (std::strcmp(frameId, "TRCK") == 0) {
            std::vector<char> content(frameSize + 1, 0);
            std::fread(content.data(), 1, frameSize, fileHandle);
            metadata.trackNumber = static_cast<uint16_t>(std::atoi(content.data() + 1));
        } else if (std::strcmp(frameId, "APIC") == 0) {
            uint32_t currentOffset = static_cast<uint32_t>(std::ftell(fileHandle));
            metadata.hasEmbeddedArtwork = true;
            metadata.embeddedArtworkOffset = currentOffset;
            metadata.embeddedArtworkSize = frameSize;
            std::fseek(fileHandle, frameSize, SEEK_CUR);
        } else {
            std::fseek(fileHandle, frameSize, SEEK_CUR);
        }

        bytesRead += frameSize;
    }

    return true;
}
