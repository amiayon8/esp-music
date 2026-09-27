#include "epaper_driver.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

EPaperDriver::EPaperDriver(gpio_num_t sckPin, gpio_num_t mosiPin, gpio_num_t csPin,
                           gpio_num_t dcPin, gpio_num_t resetPin, gpio_num_t busyPin)
    : pinSck(sckPin), pinMosi(mosiPin), pinCs(csPin), pinDc(dcPin),
      pinReset(resetPin), pinBusy(busyPin), spiDeviceHandle(nullptr),
      isInitialized(false), isSleeping(false), fullRefreshCounter(0) {
}

EPaperDriver::~EPaperDriver() {
    if (spiDeviceHandle != nullptr) {
        spi_bus_remove_device(spiDeviceHandle);
        spiDeviceHandle = nullptr;
    }
}

void EPaperDriver::initializeGpio() {
    gpio_config_t outputConfiguration = {};
    outputConfiguration.pin_bit_mask = (1ULL << pinCs) | (1ULL << pinDc) | (1ULL << pinReset);
    outputConfiguration.mode = GPIO_MODE_OUTPUT;
    outputConfiguration.pull_up_en = GPIO_PULLUP_ENABLE;
    outputConfiguration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    outputConfiguration.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&outputConfiguration);

    gpio_config_t inputConfiguration = {};
    inputConfiguration.pin_bit_mask = (1ULL << pinBusy);
    inputConfiguration.mode = GPIO_MODE_INPUT;
    inputConfiguration.pull_up_en = GPIO_PULLUP_ENABLE;
    inputConfiguration.pull_down_en = GPIO_PULLDOWN_DISABLE;
    inputConfiguration.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&inputConfiguration);

    gpio_set_level(pinCs, 1);
    gpio_set_level(pinDc, 1);
    gpio_set_level(pinReset, 1);
}

void EPaperDriver::initializeSpi() {
    spi_bus_config_t busConfiguration = {};
    busConfiguration.mosi_io_num = pinMosi;
    busConfiguration.miso_io_num = -1;
    busConfiguration.sclk_io_num = pinSck;
    busConfiguration.quadwp_io_num = -1;
    busConfiguration.quadhd_io_num = -1;
    busConfiguration.max_transfer_sz = BufferSizeBytes + 32;

    spi_bus_initialize(SPI2_HOST, &busConfiguration, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t deviceConfiguration = {};
    deviceConfiguration.clock_speed_hz = 10 * 1000 * 1000;
    deviceConfiguration.mode = 0;
    deviceConfiguration.spics_io_num = -1;
    deviceConfiguration.queue_size = 7;

    spi_bus_add_device(SPI2_HOST, &deviceConfiguration, &spiDeviceHandle);
}

bool EPaperDriver::initialize() {
    initializeGpio();
    initializeSpi();
    wake();
    isInitialized = true;
    return true;
}

void EPaperDriver::reset() {
    gpio_set_level(pinReset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(pinReset, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(pinReset, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void EPaperDriver::wake() {
    reset();
    waitUntilIdle();

    sendCommand(0x12);
    waitUntilIdle();

    sendCommand(0x01);
    sendData((DisplayWidth - 1) & 0xFF);
    sendData(((DisplayWidth - 1) >> 8) & 0xFF);
    sendData(0x00);

    sendCommand(0x11);
    sendData(0x03);

    sendCommand(0x44);
    sendData(0x00);
    sendData((DisplayHeight / 8) - 1);

    sendCommand(0x45);
    sendData(0x00);
    sendData(0x00);
    sendData((DisplayWidth - 1) & 0xFF);
    sendData(((DisplayWidth - 1) >> 8) & 0xFF);

    sendCommand(0x3C);
    sendData(0x05);

    sendCommand(0x18);
    sendData(0x80);

    sendCommand(0x4E);
    sendData(0x00);

    sendCommand(0x4F);
    sendData(0x00);
    sendData(0x00);

    waitUntilIdle();
    isSleeping = false;
}

void EPaperDriver::sendCommand(uint8_t command) {
    gpio_set_level(pinDc, 0);
    gpio_set_level(pinCs, 0);

    spi_transaction_t transaction = {};
    transaction.length = 8;
    transaction.tx_buffer = &command;
    spi_device_polling_transmit(spiDeviceHandle, &transaction);

    gpio_set_level(pinCs, 1);
}

void EPaperDriver::sendData(uint8_t data) {
    gpio_set_level(pinDc, 1);
    gpio_set_level(pinCs, 0);

    spi_transaction_t transaction = {};
    transaction.length = 8;
    transaction.tx_buffer = &data;
    spi_device_polling_transmit(spiDeviceHandle, &transaction);

    gpio_set_level(pinCs, 1);
}

void EPaperDriver::sendDataBuffer(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        return;
    }
    gpio_set_level(pinDc, 1);
    gpio_set_level(pinCs, 0);

    spi_transaction_t transaction = {};
    transaction.length = length * 8;
    transaction.tx_buffer = data;
    spi_device_polling_transmit(spiDeviceHandle, &transaction);

    gpio_set_level(pinCs, 1);
}

void EPaperDriver::waitUntilIdle() {
    uint32_t timeoutMilliseconds = 5000;
    while (gpio_get_level(pinBusy) == 1 && timeoutMilliseconds > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeoutMilliseconds -= 10;
    }
}

void EPaperDriver::setSleep() {
    if (!isSleeping) {
        sendCommand(0x10);
        sendData(0x01);
        isSleeping = true;
    }
}

bool EPaperDriver::isAsleep() const {
    return isSleeping;
}

void EPaperDriver::displayFrame(const uint8_t* frameBuffer, RefreshMode mode) {
    if (frameBuffer == nullptr) {
        return;
    }

    if (isSleeping) {
        wake();
    }

    if (mode == RefreshMode::Full || fullRefreshCounter >= 15) {
        sendCommand(0x24);
        sendDataBuffer(frameBuffer, BufferSizeBytes);

        sendCommand(0x26);
        sendDataBuffer(frameBuffer, BufferSizeBytes);

        sendCommand(0x22);
        sendData(0xF7);
        sendCommand(0x20);
        waitUntilIdle();

        fullRefreshCounter = 0;
    } else {
        sendCommand(0x24);
        sendDataBuffer(frameBuffer, BufferSizeBytes);

        sendCommand(0x22);
        sendData(0xFF);
        sendCommand(0x20);
        waitUntilIdle();

        fullRefreshCounter++;
    }
}

void EPaperDriver::displayPartialWindow(const uint8_t* frameBuffer, uint16_t xStart, uint16_t yStart, uint16_t width, uint16_t height) {
    if (frameBuffer == nullptr) {
        return;
    }

    if (isSleeping) {
        wake();
    }

    uint16_t xEnd = xStart + width - 1;
    uint16_t yEnd = yStart + height - 1;

    sendCommand(0x44);
    sendData(yStart / 8);
    sendData(yEnd / 8);

    sendCommand(0x45);
    sendData(xStart & 0xFF);
    sendData((xStart >> 8) & 0xFF);
    sendData(xEnd & 0xFF);
    sendData((xEnd >> 8) & 0xFF);

    sendCommand(0x4E);
    sendData(yStart / 8);

    sendCommand(0x4F);
    sendData(xStart & 0xFF);
    sendData((xStart >> 8) & 0xFF);

    sendCommand(0x24);
    sendDataBuffer(frameBuffer, BufferSizeBytes);

    sendCommand(0x22);
    sendData(0x0F);
    sendCommand(0x20);
    waitUntilIdle();
}

void EPaperDriver::clearScreen(uint8_t colorValue) {
    if (isSleeping) {
        wake();
    }

    sendCommand(0x24);
    for (size_t index = 0; index < BufferSizeBytes; ++index) {
        sendData(colorValue);
    }
    sendCommand(0x26);
    for (size_t index = 0; index < BufferSizeBytes; ++index) {
        sendData(colorValue);
    }
    sendCommand(0x22);
    sendData(0xF7);
    sendCommand(0x20);
    waitUntilIdle();
}
