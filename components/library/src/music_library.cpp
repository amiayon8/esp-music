#include "music_library.hpp"
#include "library_index.hpp"
#include "storage_manager.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <set>

MusicLibrary::MusicLibrary()
    : scanState(LibraryScanState::Idle), progressCallback(nullptr), scanTaskHandle(nullptr) {
    libraryMutex = xSemaphoreCreateMutex();
}

MusicLibrary::~MusicLibrary() {
    if (libraryMutex != nullptr) {
        vSemaphoreDelete(libraryMutex);
    }
}

bool MusicLibrary::initialize() {
    std::vector<TrackMetadata> loadedTracks;
    if (LibraryIndex::loadIndex(LibraryIndex::IndexFilePath, loadedTracks)) {
        if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
            tracks = std::move(loadedTracks);
            xSemaphoreGive(libraryMutex);
            return true;
        }
    }
    return false;
}

void MusicLibrary::startScan(const std::string& musicDirectory, bool forceRescan) {
    if (scanState == LibraryScanState::Scanning) {
        return;
    }

    if (!forceRescan && !tracks.empty()) {
        return;
    }

    scanState = LibraryScanState::Scanning;
    std::string* pathParameter = new std::string(musicDirectory);
    xTaskCreatePinnedToCore(scanTaskEntry, "LibScanTask", 8192, this, 2, &scanTaskHandle, 0);
}

void MusicLibrary::scanTaskEntry(void* parameter) {
    MusicLibrary* self = static_cast<MusicLibrary*>(parameter);
    self->performScan(StorageManager::MusicDirectory);
    vTaskDelete(nullptr);
}

void MusicLibrary::performScan(const std::string& directoryPath) {
    std::vector<TrackMetadata> newlyScannedTracks;
    uint32_t scannedCount = 0;

    scanDirectoryRecursive(directoryPath, scannedCount);

    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        LibraryIndex::saveIndex(LibraryIndex::IndexFilePath, tracks);
        scanState = LibraryScanState::Completed;
        xSemaphoreGive(libraryMutex);
    }

    if (progressCallback != nullptr) {
        progressCallback(scannedCount, static_cast<uint32_t>(tracks.size()));
    }
}

void MusicLibrary::scanDirectoryRecursive(const std::string& directoryPath, uint32_t& scannedCount) {
    DIR* dir = opendir(directoryPath.c_str());
    if (dir == nullptr) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        std::string fullPath = directoryPath + "/" + entry->d_name;
        struct stat entryStat;
        if (stat(fullPath.c_str(), &entryStat) == 0) {
            if (S_ISDIR(entryStat.st_mode)) {
                scanDirectoryRecursive(fullPath, scannedCount);
            } else if (S_ISREG(entryStat.st_mode)) {
                AudioCodecType codec = MetadataExtractor::detectCodecByExtension(fullPath);
                if (codec != AudioCodecType::Unknown) {
                    TrackMetadata metadata;
                    if (MetadataExtractor::extract(fullPath, metadata)) {
                        if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
                            metadata.id = static_cast<uint32_t>(tracks.size() + 1);
                            tracks.push_back(metadata);
                            xSemaphoreGive(libraryMutex);
                        }
                        scannedCount++;
                        if (progressCallback != nullptr) {
                            progressCallback(scannedCount, static_cast<uint32_t>(tracks.size()));
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
}

LibraryScanState MusicLibrary::getScanState() const {
    return scanState;
}

void MusicLibrary::setScanProgressCallback(LibraryScanProgressCallback callback) {
    progressCallback = callback;
}

size_t MusicLibrary::getTrackCount() const {
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        size_t count = tracks.size();
        xSemaphoreGive(libraryMutex);
        return count;
    }
    return 0;
}

const TrackMetadata* MusicLibrary::getTrackById(uint32_t id) const {
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.id == id) {
                xSemaphoreGive(libraryMutex);
                return &track;
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return nullptr;
}

const TrackMetadata* MusicLibrary::getTrackByIndex(size_t index) const {
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        if (index < tracks.size()) {
            const TrackMetadata* result = &tracks[index];
            xSemaphoreGive(libraryMutex);
            return result;
        }
        xSemaphoreGive(libraryMutex);
    }
    return nullptr;
}

std::vector<const TrackMetadata*> MusicLibrary::getAllSongs() const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        result.reserve(tracks.size());
        for (const auto& track : tracks) {
            result.push_back(&track);
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<std::string> MusicLibrary::getArtists() const {
    std::set<std::string> artistSet;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (!track.artist.empty()) {
                artistSet.insert(track.artist);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return std::vector<std::string>(artistSet.begin(), artistSet.end());
}

std::vector<std::string> MusicLibrary::getAlbums() const {
    std::set<std::string> albumSet;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (!track.album.empty()) {
                albumSet.insert(track.album);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return std::vector<std::string>(albumSet.begin(), albumSet.end());
}

std::vector<std::string> MusicLibrary::getGenres() const {
    std::set<std::string> genreSet;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (!track.genre.empty()) {
                genreSet.insert(track.genre);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return std::vector<std::string>(genreSet.begin(), genreSet.end());
}

std::vector<const TrackMetadata*> MusicLibrary::getSongsByArtist(const std::string& artist) const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.artist == artist) {
                result.push_back(&track);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getSongsByAlbum(const std::string& album) const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.album == album) {
                result.push_back(&track);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getSongsByGenre(const std::string& genre) const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.genre == genre) {
                result.push_back(&track);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getFavoriteSongs() const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.isFavorite) {
                result.push_back(&track);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getRecentlyPlayed() const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.lastPlayedTimestamp > 0) {
                result.push_back(&track);
            }
        }
        std::sort(result.begin(), result.end(), [](const TrackMetadata* a, const TrackMetadata* b) {
            return a->lastPlayedTimestamp > b->lastPlayedTimestamp;
        });
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getRecentlyAdded() const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            result.push_back(&track);
        }
        std::sort(result.begin(), result.end(), [](const TrackMetadata* a, const TrackMetadata* b) {
            return a->id > b->id;
        });
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

std::vector<const TrackMetadata*> MusicLibrary::getMostPlayed() const {
    std::vector<const TrackMetadata*> result;
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (track.playCount > 0) {
                result.push_back(&track);
            }
        }
        std::sort(result.begin(), result.end(), [](const TrackMetadata* a, const TrackMetadata* b) {
            return a->playCount > b->playCount;
        });
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

static std::string toLowerString(const std::string& input) {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
    return output;
}

std::vector<const TrackMetadata*> MusicLibrary::search(const std::string& query) const {
    std::vector<const TrackMetadata*> result;
    if (query.empty()) {
        return result;
    }

    std::string lowerQuery = toLowerString(query);

    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (const auto& track : tracks) {
            if (toLowerString(track.title).find(lowerQuery) != std::string::npos ||
                toLowerString(track.artist).find(lowerQuery) != std::string::npos ||
                toLowerString(track.album).find(lowerQuery) != std::string::npos ||
                toLowerString(track.genre).find(lowerQuery) != std::string::npos) {
                result.push_back(&track);
            }
        }
        xSemaphoreGive(libraryMutex);
    }
    return result;
}

void MusicLibrary::setTrackFavorite(uint32_t trackId, bool favorite) {
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& track : tracks) {
            if (track.id == trackId) {
                track.isFavorite = favorite;
                LibraryIndex::saveIndex(LibraryIndex::IndexFilePath, tracks);
                break;
            }
        }
        xSemaphoreGive(libraryMutex);
    }
}

void MusicLibrary::recordTrackPlayed(uint32_t trackId) {
    if (xSemaphoreTake(libraryMutex, portMAX_DELAY) == pdTRUE) {
        for (auto& track : tracks) {
            if (track.id == trackId) {
                track.playCount++;
                track.lastPlayedTimestamp = esp_timer_get_time() / 1000000;
                LibraryIndex::saveIndex(LibraryIndex::IndexFilePath, tracks);
                break;
            }
        }
        xSemaphoreGive(libraryMutex);
    }
}
