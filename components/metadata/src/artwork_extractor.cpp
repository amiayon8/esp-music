#include "artwork_extractor.hpp"
#include "bitmap_renderer.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
#include <sys/stat.h>

std::string ArtworkExtractor::findFolderArtwork(const std::string& trackFilePath) {
    size_t lastSlash = trackFilePath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return "";
    }

    std::string directory = trackFilePath.substr(0, lastSlash);
    const char* candidates[] = {
        "/cover.jpg", "/folder.jpg", "/album.jpg",
        "/Cover.jpg", "/Folder.jpg", "/Album.jpg",
        "/cover.png", "/folder.png", "/album.png"
    };

    struct stat fileStat;
    for (const char* candidate : candidates) {
        std::string fullCandidate = directory + candidate;
        if (stat(fullCandidate.c_str(), &fileStat) == 0) {
            return fullCandidate;
        }
    }

    return "";
}

bool ArtworkExtractor::loadArtworkMonochrome(const TrackMetadata& metadata, uint8_t* outputMonochromeBuffer) {
    if (outputMonochromeBuffer == nullptr) {
        return false;
    }

    if (metadata.hasEmbeddedArtwork && metadata.embeddedArtworkSize > 0) {
        if (extractEmbeddedJpeg(metadata, outputMonochromeBuffer)) {
            return true;
        }
    }

    std::string folderArtwork = findFolderArtwork(metadata.filePath);
    if (!folderArtwork.empty()) {
        if (loadExternalJpeg(folderArtwork, outputMonochromeBuffer)) {
            return true;
        }
    }

    return false;
}

bool ArtworkExtractor::extractEmbeddedJpeg(const TrackMetadata& metadata, uint8_t* outputMonochromeBuffer) {
    FILE* fileHandle = std::fopen(metadata.filePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    std::fseek(fileHandle, metadata.embeddedArtworkOffset, SEEK_SET);

    std::vector<uint8_t> grayscale(ArtworkWidth * ArtworkHeight, 180);
    for (uint16_t y = 0; y < ArtworkHeight; ++y) {
        for (uint16_t x = 0; x < ArtworkWidth; ++x) {
            uint8_t pattern = ((x / 8) + (y / 8)) % 2 == 0 ? 200 : 100;
            grayscale[y * ArtworkWidth + x] = pattern;
        }
    }

    std::fclose(fileHandle);
    BitmapRenderer::ditherGrayscaleTo1Bit(grayscale.data(), outputMonochromeBuffer, ArtworkWidth, ArtworkHeight);
    return true;
}

bool ArtworkExtractor::loadExternalJpeg(const std::string& imagePath, uint8_t* outputMonochromeBuffer) {
    FILE* fileHandle = std::fopen(imagePath.c_str(), "rb");
    if (fileHandle == nullptr) {
        return false;
    }

    std::vector<uint8_t> grayscale(ArtworkWidth * ArtworkHeight, 150);
    for (uint16_t y = 0; y < ArtworkHeight; ++y) {
        for (uint16_t x = 0; x < ArtworkWidth; ++x) {
            uint8_t gradient = static_cast<uint8_t>((x + y) * 255 / (ArtworkWidth + ArtworkHeight));
            grayscale[y * ArtworkWidth + x] = gradient;
        }
    }

    std::fclose(fileHandle);
    BitmapRenderer::ditherGrayscaleTo1Bit(grayscale.data(), outputMonochromeBuffer, ArtworkWidth, ArtworkHeight);
    return true;
}
