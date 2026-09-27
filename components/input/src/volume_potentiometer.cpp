#include "volume_potentiometer.hpp"
#include <algorithm>

VolumePotentiometer::VolumePotentiometer(adc_unit_t unit, adc_channel_t channel)
    : adcUnit(unit), adcChannel(channel), adcHandle(nullptr),
      filteredAdcValue(0), lastReportedPercentage(100), isFirstRead(true) {
}

VolumePotentiometer::~VolumePotentiometer() {
    if (adcHandle != nullptr) {
        adc_oneshot_del_unit(adcHandle);
        adcHandle = nullptr;
    }
}

bool VolumePotentiometer::initialize() {
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

    return adc_oneshot_config_channel(adcHandle, adcChannel, &channelConfig) == ESP_OK;
}

bool VolumePotentiometer::update(uint8_t& outPercentage) {
    if (adcHandle == nullptr) {
        return false;
    }

    int rawValue = 0;
    if (adc_oneshot_read(adcHandle, adcChannel, &rawValue) != ESP_OK) {
        return false;
    }

    if (isFirstRead) {
        filteredAdcValue = static_cast<uint32_t>(rawValue) << 8;
        isFirstRead = false;
    } else {
        filteredAdcValue = (filteredAdcValue * 7 + (static_cast<uint32_t>(rawValue) << 8)) / 8;
    }

    uint32_t smoothedValue = filteredAdcValue >> 8;
    uint8_t calculatedPercentage = 0;

    if (smoothedValue <= 50) {
        calculatedPercentage = 0;
    } else if (smoothedValue >= 4000) {
        calculatedPercentage = 100;
    } else {
        calculatedPercentage = static_cast<uint8_t>(((smoothedValue - 50) * 100) / 3950);
    }

    int difference = std::abs(static_cast<int>(calculatedPercentage) - static_cast<int>(lastReportedPercentage));
    if (difference >= MinimumHysteresis ||
        (calculatedPercentage == 0 && lastReportedPercentage != 0) ||
        (calculatedPercentage == 100 && lastReportedPercentage != 100)) {
        lastReportedPercentage = calculatedPercentage;
        outPercentage = calculatedPercentage;
        return true;
    }

    return false;
}

uint8_t VolumePotentiometer::getCurrentPercentage() const {
    return lastReportedPercentage;
}
