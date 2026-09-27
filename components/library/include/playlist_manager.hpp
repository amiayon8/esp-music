#pragma once

#include <string>
#include <vector>
#include "metadata_extractor.hpp"

struct Playlist {
    std::string name;
    std::vector<uint32_t> trackIds;
};

class PlaylistManager {
public:
    static constexpr const char* PlaylistsDirectory = "/sdcard/Music/Playlists";

    PlaylistManager();

    bool initialize();
    std::vector<std::string> getPlaylistNames() const;
    bool createPlaylist(const std::string& name);
    bool deletePlaylist(const std::string& name);
    bool addTrackToPlaylist(const std::string& name, uint32_t trackId);
    bool removeTrackFromPlaylist(const std::string& name, uint32_t trackId);
    std::vector<uint32_t> getTrackIds(const std::string& name) const;

    bool loadPlaylistsFromStorage();
    bool savePlaylistsToStorage();

private:
    std::vector<Playlist> playlists;
};
