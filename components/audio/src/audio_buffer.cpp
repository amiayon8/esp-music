#include "audio_buffer.hpp"
#include "esp_heap_caps.h"
#include <cstring>
#include <algorithm>

AudioRingBuffer::AudioRingBuffer(size_t capacityBytes)
    : capacity(capacityBytes), buffer(nullptr), readPosition(0),
      writePosition(0), availableBytes(0), allocatedInPsram(false) {
    mutex = xSemaphoreCreateMutex();
}

AudioRingBuffer::~AudioRingBuffer() {
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
    if (mutex != nullptr) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

bool AudioRingBuffer::initialize() {
    if (buffer != nullptr) {
        return true;
    }

    buffer = static_cast<uint8_t*>(heap_caps_malloc(capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer != nullptr) {
        allocatedInPsram = true;
    } else {
        buffer = static_cast<uint8_t*>(malloc(capacity));
        allocatedInPsram = false;
    }

    if (buffer == nullptr) {
        return false;
    }

    clear();
    return true;
}

size_t AudioRingBuffer::write(const uint8_t* sourceData, size_t length) {
    if (buffer == nullptr || sourceData == nullptr || length == 0) {
        return 0;
    }

    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }

    size_t spaceAvailable = capacity - availableBytes;
    size_t bytesToWrite = std::min(length, spaceAvailable);

    size_t firstChunk = std::min(bytesToWrite, capacity - writePosition);
    std::memcpy(buffer + writePosition, sourceData, firstChunk);

    size_t secondChunk = bytesToWrite - firstChunk;
    if (secondChunk > 0) {
        std::memcpy(buffer, sourceData + firstChunk, secondChunk);
    }

    writePosition = (writePosition + bytesToWrite) % capacity;
    availableBytes += bytesToWrite;

    xSemaphoreGive(mutex);
    return bytesToWrite;
}

size_t AudioRingBuffer::read(uint8_t* destinationData, size_t length) {
    if (buffer == nullptr || destinationData == nullptr || length == 0) {
        return 0;
    }

    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }

    size_t bytesToRead = std::min(length, availableBytes);

    size_t firstChunk = std::min(bytesToRead, capacity - readPosition);
    std::memcpy(destinationData, buffer + readPosition, firstChunk);

    size_t secondChunk = bytesToRead - firstChunk;
    if (secondChunk > 0) {
        std::memcpy(destinationData + firstChunk, buffer, secondChunk);
    }

    readPosition = (readPosition + bytesToRead) % capacity;
    availableBytes -= bytesToRead;

    xSemaphoreGive(mutex);
    return bytesToRead;
}

void AudioRingBuffer::clear() {
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        readPosition = 0;
        writePosition = 0;
        availableBytes = 0;
        xSemaphoreGive(mutex);
    }
}

size_t AudioRingBuffer::getAvailableForRead() const {
    size_t bytes = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        bytes = availableBytes;
        xSemaphoreGive(mutex);
    }
    return bytes;
}

size_t AudioRingBuffer::getAvailableForWrite() const {
    size_t bytes = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        bytes = capacity - availableBytes;
        xSemaphoreGive(mutex);
    }
    return bytes;
}

size_t AudioRingBuffer::getCapacity() const {
    return capacity;
}
