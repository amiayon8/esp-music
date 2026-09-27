#pragma once

#include <cstdint>
#include <string>
#include "metadata_extractor.hpp"

class ArtworkExtractor {
public:
    static constexpr uint16_t ArtworkWidth = 120;
    static constexpr uint16_t ArtworkHeight = 120;
    static constexpr size_t ArtworkBufferSize = (ArtworkWidth * ArtworkHeight) / 8;

    static bool loadArtworkMonochrome(const TrackMetadata& metadata, uint8_t* outputMonochromeBuffer);
    static std::string findFolderArtwork(const std::string& trackFilePath);

private:
    static bool extractEmbeddedJpeg(const TrackMetadata& metadata, uint8_t* outputMonochromeBuffer);
    static bool loadExternalJpeg(const std::string& imagePath, uint8_t* outputMonochromeBuffer);
};
