#pragma once

#include <cstdio>
#include "metadata_extractor.hpp"

class VorbisParser {
public:
    static bool parseFlac(FILE* fileHandle, TrackMetadata& metadata);
    static bool parseOggOpus(FILE* fileHandle, TrackMetadata& metadata);

private:
    static void parseVorbisCommentBlock(const uint8_t* buffer, size_t length, TrackMetadata& metadata);
    static void parseVorbisEntry(const std::string& key, const std::string& value, TrackMetadata& metadata);
};
