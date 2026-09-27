#pragma once

#include <cstdint>
#include <cstddef>
#include "driver/spi_master.h"
#include "driver/gpio.h"

class EPaperDriver {
public:
    static constexpr uint16_t DisplayWidth = 296;
    static constexpr uint16_t DisplayHeight = 128;
    static constexpr size_t BufferSizeBytes = (DisplayWidth * DisplayHeight) / 8;

    enum class RefreshMode {
        Full,
        Partial
    };

    EPaperDriver(gpio_num_t sckPin, gpio_num_t mosiPin, gpio_num_t csPin,
                 gpio_num_t dcPin, gpio_num_t resetPin, gpio_num_t busyPin);
    ~EPaperDriver();

    bool initialize();
    void reset();
    void wake();
    void setSleep();
    bool isAsleep() const;

    void sendCommand(uint8_t command);
    void sendData(uint8_t data);
    void sendDataBuffer(const uint8_t* data, size_t length);
    void waitUntilIdle();
    
    void displayFrame(const uint8_t* frameBuffer, RefreshMode mode);
    void displayPartialWindow(const uint8_t* frameBuffer, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height);
    void clearScreen(uint8_t colorValue = 0xFF);

private:
    gpio_num_t pinSck;
    gpio_num_t pinMosi;
    gpio_num_t pinCs;
    gpio_num_t pinDc;
    gpio_num_t pinReset;
    gpio_num_t pinBusy;
    spi_device_handle_t spiDeviceHandle;
    bool isInitialized;
    bool isSleeping;
    uint32_t fullRefreshCounter;

    void initializeGpio();
    void initializeSpi();
};
