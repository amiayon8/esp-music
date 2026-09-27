#include "playlist_manager.hpp"
#include <algorithm>

PlaylistManager::PlaylistManager() {
    createPlaylist("Favourite Songs");
    createPlaylist("DJ");
    createPlaylist("Knight Ride");
    createPlaylist("Durga Puja Slow");
    createPlaylist("Shaadi Slow");
    createPlaylist("Truck Driver");
    createPlaylist("Gym");
}

bool PlaylistManager::initialize() {
    return loadPlaylistsFromStorage();
}

std::vector<std::string> PlaylistManager::getPlaylistNames() const {
    std::vector<std::string> names;
    names.reserve(playlists.size());
    for (const auto& playlist : playlists) {
        names.push_back(playlist.name);
    }
    return names;
}

bool PlaylistManager::createPlaylist(const std::string& name) {
    for (const auto& playlist : playlists) {
        if (playlist.name == name) {
            return false;
        }
    }
    Playlist newPlaylist;
    newPlaylist.name = name;
    playlists.push_back(newPlaylist);
    return true;
}

bool PlaylistManager::deletePlaylist(const std::string& name) {
    auto iterator = std::remove_if(playlists.begin(), playlists.end(),
        [&name](const Playlist& playlist) {
            return playlist.name == name;
        });

    if (iterator != playlists.end()) {
        playlists.erase(iterator, playlists.end());
        return true;
    }
    return false;
}

bool PlaylistManager::addTrackToPlaylist(const std::string& name, uint32_t trackId) {
    for (auto& playlist : playlists) {
        if (playlist.name == name) {
            for (uint32_t id : playlist.trackIds) {
                if (id == trackId) {
                    return false;
                }
            }
            playlist.trackIds.push_back(trackId);
            return true;
        }
    }
    return false;
}

bool PlaylistManager::removeTrackFromPlaylist(const std::string& name, uint32_t trackId) {
    for (auto& playlist : playlists) {
        if (playlist.name == name) {
            auto iterator = std::remove(playlist.trackIds.begin(), playlist.trackIds.end(), trackId);
            if (iterator != playlist.trackIds.end()) {
                playlist.trackIds.erase(iterator, playlist.trackIds.end());
                return true;
            }
            return false;
        }
    }
    return false;
}

std::vector<uint32_t> PlaylistManager::getTrackIds(const std::string& name) const {
    for (const auto& playlist : playlists) {
        if (playlist.name == name) {
            return playlist.trackIds;
        }
    }
    return {};
}

bool PlaylistManager::loadPlaylistsFromStorage() {
    return true;
}

bool PlaylistManager::savePlaylistsToStorage() {
    return true;
}
