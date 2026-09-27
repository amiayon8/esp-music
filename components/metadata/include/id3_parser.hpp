#pragma once

#include <cstdio>
#include "metadata_extractor.hpp"

class Id3Parser {
public:
    static bool parse(FILE* fileHandle, TrackMetadata& metadata);

private:
    static bool parseId3v1(FILE* fileHandle, TrackMetadata& metadata);
    static bool parseId3v2(FILE* fileHandle, TrackMetadata& metadata);
    static uint32_t decodeSynchsafeInteger(const uint8_t* bytes);
};
