#include "power_manager.hpp"
#include "esp_sleep.h"
#include "esp_timer.h"
#include <algorithm>

PowerManager::PowerManager(adc_unit_t unit, adc_channel_t channel)
    : adcUnit(unit), adcChannel(channel), adcHandle(nullptr),
      batteryPercentage(100), batteryVoltageMv(4200),
      idleTimeoutSeconds(DefaultIdleTimeoutSeconds),
      lastUserActivityTimestampMs(0), lowBatteryCallback(nullptr),
      lowBatteryNotified(false) {
}

PowerManager::~PowerManager() {
    if (adcHandle != nullptr) {
        adc_oneshot_del_unit(adcHandle);
        adcHandle = nullptr;
    }
}

bool PowerManager::initialize() {
    adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = adcUnit,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };

    if (adc_oneshot_new_unit(&unitConfig, &adcHandle) != ESP_OK) {
        return false;
    }

    adc_oneshot_chan_cfg_t channelConfig = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };

    if (adc_oneshot_config_channel(adcHandle, adcChannel, &channelConfig) != ESP_OK) {
        return false;
    }

    lastUserActivityTimestampMs = esp_timer_get_time() / 1000;
    update();
    return true;
}

void PowerManager::update() {
    if (adcHandle == nullptr) {
        return;
    }

    int rawValue = 0;
    if (adc_oneshot_read(adcHandle, adcChannel, &rawValue) != ESP_OK) {
        return;
    }

    uint32_t adcVoltage = (rawValue * 3300) / 4095;
    batteryVoltageMv = adcVoltage * 2;

    if (batteryVoltageMv <= 3200) {
        batteryPercentage = 0;
    } else if (batteryVoltageMv >= 4200) {
        batteryPercentage = 100;
    } else {
        batteryPercentage = static_cast<uint8_t>(((batteryVoltageMv - 3200) * 100) / 1000);
    }

    if (isLowBattery() && !lowBatteryNotified) {
        lowBatteryNotified = true;
        if (lowBatteryCallback != nullptr) {
            lowBatteryCallback();
        }
    } else if (!isLowBattery()) {
        lowBatteryNotified = false;
    }
}

uint8_t PowerManager::getBatteryPercentage() const {
    return batteryPercentage;
}

uint32_t PowerManager::getBatteryVoltageMv() const {
    return batteryVoltageMv;
}

bool PowerManager::isLowBattery() const {
    return batteryPercentage <= LowBatteryThresholdPercentage;
}

void PowerManager::resetIdleTimer() {
    lastUserActivityTimestampMs = esp_timer_get_time() / 1000;
}

bool PowerManager::hasIdleTimedOut() const {
    if (idleTimeoutSeconds == 0) {
        return false;
    }
    uint64_t nowMs = esp_timer_get_time() / 1000;
    return (nowMs - lastUserActivityTimestampMs) >= (idleTimeoutSeconds * 1000);
}

void PowerManager::setIdleTimeoutSeconds(uint32_t seconds) {
    idleTimeoutSeconds = seconds;
}

void PowerManager::setLowBatteryCallback(LowBatteryCallback callback) {
    lowBatteryCallback = callback;
}

void PowerManager::enterDeepSleep() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}
