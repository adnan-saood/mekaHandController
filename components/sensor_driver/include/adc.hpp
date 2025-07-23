#pragma once
#include "sensors.hpp"

class ADC {
public:
    ADCData read() {
        ADCData data;
        // Read ADC data via SPI
        for (size_t i = 0; i < NUM_MOTORS; ++i) {
            data.values[i] = 0.0f; // Replace with actual reading logic
        }
        return data;
    }
};
