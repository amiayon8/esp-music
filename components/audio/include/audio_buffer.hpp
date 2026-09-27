#pragma once

#include <cstdint>
#include <cstddef>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacityBytes);
    ~AudioRingBuffer();

    bool initialize();
    size_t write(const uint8_t* sourceData, size_t length);
    size_t read(uint8_t* destinationData, size_t length);
    void clear();

    size_t getAvailableForRead() const;
    size_t getAvailableForWrite() const;
    size_t getCapacity() const;

private:
    size_t capacity;
    uint8_t* buffer;
    size_t readPosition;
    size_t writePosition;
    size_t availableBytes;
    SemaphoreHandle_t mutex;
    bool allocatedInPsram;
};
