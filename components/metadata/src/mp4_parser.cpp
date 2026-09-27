#include "mp4_parser.hpp"
#include <vector>
#include <cstring>

static uint32_t readBigEndian32(FILE* fileHandle) {
    uint8_t buffer[4];
    if (std::fread(buffer, 1, 4, fileHandle) != 4) {
        return 0;
    }
    return (static_cast<uint32_t>(buffer[0]) << 24) |
           (static_cast<uint32_t>(buffer[1]) << 16) |
           (static_cast<uint32_t>(buffer[2]) << 8) |
           static_cast<uint32_t>(buffer[3]);
}

bool Mp4Parser::parse(FILE* fileHandle, TrackMetadata& metadata) {
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, 0, SEEK_SET);

    while (!std::feof(fileHandle)) {
        uint32_t atomSize = readBigEndian32(fileHandle);
        if (atomSize < 8) {
            break;
        }

        uint32_t atomType = readBigEndian32(fileHandle);
        if (atomType == 0x6D6F6F76) {
            uint32_t bytesRead = 8;
            while (bytesRead < atomSize && !std::feof(fileHandle)) {
                uint32_t subSize = readBigEndian32(fileHandle);
                if (subSize < 8) {
                    break;
                }
                uint32_t subType = readBigEndian32(fileHandle);
                bytesRead += subSize;

                if (subType == 0x75647461) {
                    uint32_t udtaBytes = 8;
                    while (udtaBytes < subSize && !std::feof(fileHandle)) {
                        uint32_t metaSize = readBigEndian32(fileHandle);
                        if (metaSize < 8) {
                            break;
                        }
                        uint32_t metaType = readBigEndian32(fileHandle);
                        udtaBytes += metaSize;

                        if (metaType == 0x6D657461) {
                            std::fseek(fileHandle, 4, SEEK_CUR);
                            uint32_t metaInnerBytes = 12;
                            while (metaInnerBytes < metaSize && !std::feof(fileHandle)) {
                                uint32_t ilstSize = readBigEndian32(fileHandle);
                                if (ilstSize < 8) {
                                    break;
                                }
                                uint32_t ilstType = readBigEndian32(fileHandle);
                                metaInnerBytes += ilstSize;

                                if (ilstType == 0x696C7374) {
                                    parseIlstBox(fileHandle, ilstSize - 8, metadata);
                                    return true;
                                } else {
                                    std::fseek(fileHandle, ilstSize - 8, SEEK_CUR);
                                }
                            }
                        } else {
                            std::fseek(fileHandle, metaSize - 8, SEEK_CUR);
                        }
                    }
                } else {
                    std::fseek(fileHandle, subSize - 8, SEEK_CUR);
                }
            }
            break;
        } else {
            std::fseek(fileHandle, atomSize - 8, SEEK_CUR);
        }
    }

    return true;
}

void Mp4Parser::parseIlstBox(FILE* fileHandle, uint32_t boxSize, TrackMetadata& metadata) {
    uint32_t bytesProcessed = 0;
    while (bytesProcessed + 8 <= boxSize && !std::feof(fileHandle)) {
        uint32_t itemSize = readBigEndian32(fileHandle);
        if (itemSize < 8 || bytesProcessed + itemSize > boxSize) {
            break;
        }
        uint32_t itemType = readBigEndian32(fileHandle);
        bytesProcessed += itemSize;

        parseAtomData(fileHandle, itemType, itemSize - 8, metadata);
    }
}

void Mp4Parser::parseAtomData(FILE* fileHandle, uint32_t fourCc, uint32_t atomSize, TrackMetadata& metadata) {
    if (atomSize < 16) {
        std::fseek(fileHandle, atomSize, SEEK_CUR);
        return;
    }

    uint32_t dataBoxSize = readBigEndian32(fileHandle);
    uint32_t dataBoxType = readBigEndian32(fileHandle);

    if (dataBoxType != 0x64617461 || dataBoxSize < 16) {
        std::fseek(fileHandle, atomSize - 8, SEEK_CUR);
        return;
    }

    std::fseek(fileHandle, 8, SEEK_CUR);
    uint32_t payloadLength = dataBoxSize - 16;

    if (fourCc == 0xA96E616D) {
        std::vector<char> text(payloadLength + 1, 0);
        std::fread(text.data(), 1, payloadLength, fileHandle);
        metadata.title = text.data();
    } else if (fourCc == 0xA9415254) {
        std::vector<char> text(payloadLength + 1, 0);
        std::fread(text.data(), 1, payloadLength, fileHandle);
        metadata.artist = text.data();
    } else if (fourCc == 0xA9616C62) {
        std::vector<char> text(payloadLength + 1, 0);
        std::fread(text.data(), 1, payloadLength, fileHandle);
        metadata.album = text.data();
    } else if (fourCc == 0xA967656E) {
        std::vector<char> text(payloadLength + 1, 0);
        std::fread(text.data(), 1, payloadLength, fileHandle);
        metadata.genre = text.data();
    } else if (fourCc == 0x74726B6E && payloadLength >= 4) {
        uint8_t trackBuffer[4];
        std::fread(trackBuffer, 1, 4, fileHandle);
        metadata.trackNumber = (static_cast<uint16_t>(trackBuffer[2]) << 8) | trackBuffer[3];
        if (payloadLength > 4) {
            std::fseek(fileHandle, payloadLength - 4, SEEK_CUR);
        }
    } else if (fourCc == 0x636F7672) {
        uint32_t currentOffset = static_cast<uint32_t>(std::ftell(fileHandle));
        metadata.hasEmbeddedArtwork = true;
        metadata.embeddedArtworkOffset = currentOffset;
        metadata.embeddedArtworkSize = payloadLength;
        std::fseek(fileHandle, payloadLength, SEEK_CUR);
    } else {
        std::fseek(fileHandle, payloadLength, SEEK_CUR);
    }

    if (atomSize > dataBoxSize) {
        std::fseek(fileHandle, atomSize - dataBoxSize, SEEK_CUR);
    }
}
