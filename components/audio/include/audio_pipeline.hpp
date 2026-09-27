#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include "audio_decoder.hpp"
#include "audio_buffer.hpp"
#include "volume_scaler.hpp"
#include "audio_resampler.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum class AudioPipelineStatus {
    Stopped,
    Playing,
    Paused,
    Error
};

using AudioTrackFinishedCallback = std::function<void()>;
using AudioPositionUpdatedCallback = std::function<void(uint32_t currentSeconds, uint32_t totalSeconds)>;

class AudioPipeline {
public:
    static constexpr size_t RingBufferCapacity = 128 * 1024;
    static constexpr uint32_t TargetSampleRate = 44100;

    AudioPipeline();
    ~AudioPipeline();

    bool initialize();
    bool playTrack(const std::string& filePath, AudioCodecType codec);
    void pause();
    void resume();
    void stop();
    bool seek(uint32_t targetSeconds);

    void prepareNextTrack(const std::string& filePath, AudioCodecType codec);

    void setVolume(uint8_t percentage);
    uint8_t getVolume() const;

    AudioPipelineStatus getStatus() const;
    uint32_t getCurrentPositionSeconds() const;
    uint32_t getDurationSeconds() const;
    uint32_t getSampleRate() const;
    uint8_t getChannels() const;
    uint8_t getBitDepth() const;

    size_t pullPcmDataForBluetooth(uint8_t* destinationBuffer, size_t requestedBytes);

    void setTrackFinishedCallback(AudioTrackFinishedCallback callback);
    void setPositionCallback(AudioPositionUpdatedCallback callback);

private:
    std::unique_ptr<AudioDecoder> activeDecoder;
    std::unique_ptr<AudioDecoder> preparedDecoder;
    AudioRingBuffer ringBuffer;
    VolumeScaler volumeScaler;
    AudioResampler resampler;

    AudioPipelineStatus status;
    TaskHandle_t decodeTaskHandle;
    bool isTaskRunning;

    AudioTrackFinishedCallback trackFinishedCallback;
    AudioPositionUpdatedCallback positionCallback;

    static void decodeTaskEntry(void* parameter);
    void decodeLoop();
};
