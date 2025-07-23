#pragma once

#include <array>
#include "config.h"

struct IMUData {
    std::array<float, 3> accel; // e.g., [accel_x, accel_y, accel_z]
    std::array<float, 3> gyro;  // e.g., [gyro_x, gyro_y, gyro_z]
};

struct ADCData {
    std::array<float, NUM_MOTORS> values; // e.g., one ADC value per motor
};

struct EncoderData {
    std::array<float, NUM_MOTORS> position;
    std::array<float, NUM_MOTORS> velocity;
};
