#pragma once

#include <cstdint>
#include "esp_adc/adc_oneshot.h"

class VolumePotentiometer {
public:
    static constexpr uint8_t MinimumHysteresis = 2;

    VolumePotentiometer(adc_unit_t unit, adc_channel_t channel);
    ~VolumePotentiometer();

    bool initialize();
    bool update(uint8_t& outPercentage);
    uint8_t getCurrentPercentage() const;

private:
    adc_unit_t adcUnit;
    adc_channel_t adcChannel;
    adc_oneshot_unit_handle_t adcHandle;
    uint32_t filteredAdcValue;
    uint8_t lastReportedPercentage;
    bool isFirstRead;
};
