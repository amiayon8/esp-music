#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

namespace HardwareConfig {

inline constexpr gpio_num_t DisplaySpiClock = GPIO_NUM_18;
inline constexpr gpio_num_t DisplaySpiMosi = GPIO_NUM_23;
inline constexpr gpio_num_t DisplayChipSelect = GPIO_NUM_5;
inline constexpr gpio_num_t DisplayDataCommand = GPIO_NUM_22;
inline constexpr gpio_num_t DisplayReset = GPIO_NUM_21;
inline constexpr gpio_num_t DisplayBusy = GPIO_NUM_4;

inline constexpr gpio_num_t StorageSpiClock = GPIO_NUM_14;
inline constexpr gpio_num_t StorageSpiMosi = GPIO_NUM_13;
inline constexpr gpio_num_t StorageSpiMiso = GPIO_NUM_12;
inline constexpr gpio_num_t StorageChipSelect = GPIO_NUM_15;
inline constexpr gpio_num_t StorageCardDetect = GPIO_NUM_27;

inline constexpr gpio_num_t ButtonUp = GPIO_NUM_32;
inline constexpr gpio_num_t ButtonDown = GPIO_NUM_33;
inline constexpr gpio_num_t ButtonLeft = GPIO_NUM_25;
inline constexpr gpio_num_t ButtonRight = GPIO_NUM_26;
inline constexpr gpio_num_t ButtonEnter = GPIO_NUM_19;
inline constexpr gpio_num_t ButtonBluetooth = GPIO_NUM_2;
inline constexpr gpio_num_t ButtonLibrary = GPIO_NUM_17;
inline constexpr gpio_num_t ButtonBack = GPIO_NUM_0;

inline constexpr gpio_num_t EncoderPhaseA = GPIO_NUM_36;
inline constexpr gpio_num_t EncoderPhaseB = GPIO_NUM_39;
inline constexpr gpio_num_t EncoderSwitch = GPIO_NUM_19;

inline constexpr adc_channel_t VolumePotentiometerAdcChannel = ADC_CHANNEL_6;
inline constexpr gpio_num_t VolumePotentiometerPin = GPIO_NUM_34;

inline constexpr adc_channel_t BatteryMonitorAdcChannel = ADC_CHANNEL_7;
inline constexpr gpio_num_t BatteryMonitorPin = GPIO_NUM_35;

}
