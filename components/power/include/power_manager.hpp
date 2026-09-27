#pragma once

#include <cstdint>
#include <functional>
#include "esp_adc/adc_oneshot.h"

using LowBatteryCallback = std::function<void()>;

class PowerManager {
public:
    static constexpr uint32_t DefaultIdleTimeoutSeconds = 300;
    static constexpr uint8_t LowBatteryThresholdPercentage = 5;

    PowerManager(adc_unit_t unit, adc_channel_t channel);
    ~PowerManager();

    bool initialize();
    void update();

    uint8_t getBatteryPercentage() const;
    uint32_t getBatteryVoltageMv() const;
    bool isLowBattery() const;

    void resetIdleTimer();
    bool hasIdleTimedOut() const;
    void setIdleTimeoutSeconds(uint32_t seconds);

    void setLowBatteryCallback(LowBatteryCallback callback);
    void enterDeepSleep();

private:
    adc_unit_t adcUnit;
    adc_channel_t adcChannel;
    adc_oneshot_unit_handle_t adcHandle;

    uint8_t batteryPercentage;
    uint32_t batteryVoltageMv;
    uint32_t idleTimeoutSeconds;
    uint64_t lastUserActivityTimestampMs;
    LowBatteryCallback lowBatteryCallback;
    bool lowBatteryNotified;
};
