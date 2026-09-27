#include "resume_manager.hpp"
#include "nvs_flash.h"
#include "esp_timer.h"

ResumeManager::ResumeManager()
    : lastSaveTimestampSeconds(0) {
}

bool ResumeManager::loadState(ResumeData& data) {
    nvs_handle_t handle;
    if (nvs_open("playback", NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    uint32_t trackId = 0;
    uint32_t position = 0;
    uint8_t repeatInt = 0;
    uint8_t shuffleInt = 0;
    uint8_t volume = 100;

    if (nvs_get_u32(handle, "track_id", &trackId) != ESP_OK ||
        nvs_get_u32(handle, "position", &position) != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    nvs_get_u8(handle, "repeat", &repeatInt);
    nvs_get_u8(handle, "shuffle", &shuffleInt);
    nvs_get_u8(handle, "volume", &volume);

    data.trackId = trackId;
    data.positionSeconds = position;
    data.repeatMode = static_cast<RepeatMode>(repeatInt);
    data.shuffleMode = static_cast<ShuffleMode>(shuffleInt);
    data.volumePercentage = volume;

    nvs_close(handle);
    return true;
}

void ResumeManager::saveState(const ResumeData& data, bool forceImmediate) {
    uint64_t nowSeconds = esp_timer_get_time() / 1000000;

    if (!forceImmediate && (nowSeconds - lastSaveTimestampSeconds) < SaveIntervalSeconds) {
        return;
    }

    nvs_handle_t handle;
    if (nvs_open("playback", NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    nvs_set_u32(handle, "track_id", data.trackId);
    nvs_set_u32(handle, "position", data.positionSeconds);
    nvs_set_u8(handle, "repeat", static_cast<uint8_t>(data.repeatMode));
    nvs_set_u8(handle, "shuffle", static_cast<uint8_t>(data.shuffleMode));
    nvs_set_u8(handle, "volume", data.volumePercentage);

    nvs_commit(handle);
    nvs_close(handle);

    lastSaveTimestampSeconds = nowSeconds;
}
