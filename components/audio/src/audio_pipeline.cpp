#include "audio_pipeline.hpp"
#include "esp_log.h"
#include <vector>

AudioPipeline::AudioPipeline()
    : ringBuffer(RingBufferCapacity), status(AudioPipelineStatus::Stopped),
      decodeTaskHandle(nullptr), isTaskRunning(false),
      trackFinishedCallback(nullptr), positionCallback(nullptr) {
}

AudioPipeline::~AudioPipeline() {
    stop();
    isTaskRunning = false;
    if (decodeTaskHandle != nullptr) {
        vTaskDelete(decodeTaskHandle);
        decodeTaskHandle = nullptr;
    }
}

bool AudioPipeline::initialize() {
    if (!ringBuffer.initialize()) {
        return false;
    }

    isTaskRunning = true;
    xTaskCreatePinnedToCore(decodeTaskEntry, "AudioDecode", 8192, this, 6, &decodeTaskHandle, 1);
    return true;
}

void AudioPipeline::decodeTaskEntry(void* parameter) {
    AudioPipeline* self = static_cast<AudioPipeline*>(parameter);
    self->decodeLoop();
    vTaskDelete(nullptr);
}

void AudioPipeline::decodeLoop() {
    constexpr size_t SampleBatchSize = 1024;
    std::vector<int16_t> pcmBatch(SampleBatchSize * 2);
    std::vector<int16_t> resampledBatch(SampleBatchSize * 4);

    while (isTaskRunning) {
        if (status != AudioPipelineStatus::Playing || activeDecoder == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        size_t availableSpace = ringBuffer.getAvailableForWrite();
        size_t requiredSpace = SampleBatchSize * 2 * sizeof(int16_t);

        if (availableSpace < requiredSpace) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        size_t samplesDecoded = 0;
        DecoderStatus decodeResult = activeDecoder->decode(pcmBatch.data(), SampleBatchSize * 2, samplesDecoded);

        if (decodeResult == DecoderStatus::Success && samplesDecoded > 0) {
            size_t outputSampleCount = 0;
            const int16_t* finalPcmData = pcmBatch.data();

            if (activeDecoder->getSampleRate() != TargetSampleRate) {
                outputSampleCount = resampler.process(pcmBatch.data(), samplesDecoded, resampledBatch.data(), resampledBatch.size());
                finalPcmData = resampledBatch.data();
            } else {
                outputSampleCount = samplesDecoded;
            }

            volumeScaler.process(const_cast<int16_t*>(finalPcmData), outputSampleCount);
            ringBuffer.write(reinterpret_cast<const uint8_t*>(finalPcmData), outputSampleCount * sizeof(int16_t));

            if (positionCallback != nullptr) {
                positionCallback(activeDecoder->getCurrentPositionSeconds(), activeDecoder->getDurationSeconds());
            }
        } else if (decodeResult == DecoderStatus::EndOfFile) {
            if (preparedDecoder != nullptr) {
                activeDecoder = std::move(preparedDecoder);
                preparedDecoder = nullptr;
                resampler.configure(activeDecoder->getSampleRate(), TargetSampleRate, activeDecoder->getChannels());
            } else {
                status = AudioPipelineStatus::Stopped;
                if (trackFinishedCallback != nullptr) {
                    trackFinishedCallback();
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

bool AudioPipeline::playTrack(const std::string& filePath, AudioCodecType codec) {
    std::unique_ptr<AudioDecoder> newDecoder = AudioDecoder::createDecoder(codec);
    if (newDecoder == nullptr || !newDecoder->open(filePath)) {
        return false;
    }

    ringBuffer.clear();
    resampler.configure(newDecoder->getSampleRate(), TargetSampleRate, newDecoder->getChannels());
    activeDecoder = std::move(newDecoder);
    status = AudioPipelineStatus::Playing;
    return true;
}

void AudioPipeline::prepareNextTrack(const std::string& filePath, AudioCodecType codec) {
    std::unique_ptr<AudioDecoder> nextDecoder = AudioDecoder::createDecoder(codec);
    if (nextDecoder != nullptr && nextDecoder->open(filePath)) {
        preparedDecoder = std::move(nextDecoder);
    }
}

void AudioPipeline::pause() {
    if (status == AudioPipelineStatus::Playing) {
        status = AudioPipelineStatus::Paused;
    }
}

void AudioPipeline::resume() {
    if (status == AudioPipelineStatus::Paused && activeDecoder != nullptr) {
        status = AudioPipelineStatus::Playing;
    }
}

void AudioPipeline::stop() {
    status = AudioPipelineStatus::Stopped;
    ringBuffer.clear();
    activeDecoder.reset();
    preparedDecoder.reset();
}

bool AudioPipeline::seek(uint32_t targetSeconds) {
    if (activeDecoder == nullptr) {
        return false;
    }
    ringBuffer.clear();
    return activeDecoder->seek(targetSeconds);
}

void AudioPipeline::setVolume(uint8_t percentage) {
    volumeScaler.setVolumePercentage(percentage);
}

uint8_t AudioPipeline::getVolume() const {
    return volumeScaler.getVolumePercentage();
}

AudioPipelineStatus AudioPipeline::getStatus() const {
    return status;
}

uint32_t AudioPipeline::getCurrentPositionSeconds() const {
    if (activeDecoder == nullptr) {
        return 0;
    }
    return activeDecoder->getCurrentPositionSeconds();
}

uint32_t AudioPipeline::getDurationSeconds() const {
    if (activeDecoder == nullptr) {
        return 0;
    }
    return activeDecoder->getDurationSeconds();
}

uint32_t AudioPipeline::getSampleRate() const {
    if (activeDecoder == nullptr) {
        return TargetSampleRate;
    }
    return activeDecoder->getSampleRate();
}

uint8_t AudioPipeline::getChannels() const {
    if (activeDecoder == nullptr) {
        return 2;
    }
    return activeDecoder->getChannels();
}

uint8_t AudioPipeline::getBitDepth() const {
    if (activeDecoder == nullptr) {
        return 16;
    }
    return activeDecoder->getBitDepth();
}

size_t AudioPipeline::pullPcmDataForBluetooth(uint8_t* destinationBuffer, size_t requestedBytes) {
    if (status != AudioPipelineStatus::Playing) {
        return 0;
    }
    return ringBuffer.read(destinationBuffer, requestedBytes);
}

void AudioPipeline::setTrackFinishedCallback(AudioTrackFinishedCallback callback) {
    trackFinishedCallback = callback;
}

void AudioPipeline::setPositionCallback(AudioPositionUpdatedCallback callback) {
    positionCallback = callback;
}
