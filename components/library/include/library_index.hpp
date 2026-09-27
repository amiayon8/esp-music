#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "metadata_extractor.hpp"

struct LibraryHeader {
    char magic[4];
    uint32_t version;
    uint32_t trackCount;
    uint64_t lastScanTimestamp;
};

class LibraryIndex {
public:
    static constexpr const char* IndexFilePath = "/sdcard/Music/.library.idx";

    static bool saveIndex(const std::string& indexFilePath, const std::vector<TrackMetadata>& tracks);
    static bool loadIndex(const std::string& indexFilePath, std::vector<TrackMetadata>& tracks);
};
