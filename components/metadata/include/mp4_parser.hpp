#pragma once

#include <cstdio>
#include "metadata_extractor.hpp"

class Mp4Parser {
public:
    static bool parse(FILE* fileHandle, TrackMetadata& metadata);

private:
    static void parseIlstBox(FILE* fileHandle, uint32_t boxSize, TrackMetadata& metadata);
    static void parseAtomData(FILE* fileHandle, uint32_t fourCc, uint32_t atomSize, TrackMetadata& metadata);
};
