#pragma once

#include <cstdint>

enum class RepeatMode : uint8_t {
    Off = 0,
    All,
    One
};

enum class ShuffleMode : uint8_t {
    Off = 0,
    On
};
