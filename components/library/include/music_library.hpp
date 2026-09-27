#pragma once

#include <vector>
#include <string>
#include <functional>
#include "metadata_extractor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

enum class LibraryScanState {
    Idle,
    Scanning,
    Completed,
    Failed
};

using LibraryScanProgressCallback = std::function<void(uint32_t currentFiles, uint32_t totalTracks)>;

class MusicLibrary {
public:
    MusicLibrary();
    ~MusicLibrary();

    bool initialize();
    void startScan(const std::string& musicDirectory, bool forceRescan = false);
    LibraryScanState getScanState() const;
    void setScanProgressCallback(LibraryScanProgressCallback callback);

    size_t getTrackCount() const;
    const TrackMetadata* getTrackById(uint32_t id) const;
    const TrackMetadata* getTrackByIndex(size_t index) const;

    std::vector<const TrackMetadata*> getAllSongs() const;
    std::vector<std::string> getArtists() const;
    std::vector<std::string> getAlbums() const;
    std::vector<std::string> getGenres() const;
    std::vector<std::string> getPlaylists() const;

    std::vector<const TrackMetadata*> getSongsByArtist(const std::string& artist) const;
    std::vector<const TrackMetadata*> getSongsByAlbum(const std::string& album) const;
    std::vector<const TrackMetadata*> getSongsByGenre(const std::string& genre) const;
    std::vector<const TrackMetadata*> getFavoriteSongs() const;
    std::vector<const TrackMetadata*> getRecentlyPlayed() const;
    std::vector<const TrackMetadata*> getRecentlyAdded() const;
    std::vector<const TrackMetadata*> getMostPlayed() const;

    std::vector<const TrackMetadata*> search(const std::string& query) const;

    void setTrackFavorite(uint32_t trackId, bool favorite);
    void recordTrackPlayed(uint32_t trackId);

private:
    std::vector<TrackMetadata> tracks;
    mutable SemaphoreHandle_t libraryMutex;
    LibraryScanState scanState;
    LibraryScanProgressCallback progressCallback;
    TaskHandle_t scanTaskHandle;

    static void scanTaskEntry(void* parameter);
    void performScan(const std::string& directoryPath);
    void scanDirectoryRecursive(const std::string& directoryPath, uint32_t& scannedCount);
};
