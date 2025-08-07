#pragma once

#include <array>
#include "config.h"

#define ADC_CHANNELS 16

struct IMUData {
    std::array<float, 3> accel; // e.g., [accel_x, accel_y, accel_z]
    std::array<float, 3> gyro;  // e.g., [gyro_x, gyro_y, gyro_z]
    std::array<float, 4> quaternion; // e.g., [q0, q1, q2, q3]
};

struct ADCData {
    std::array<float, ADC_CHANNELS> values; // e.g., one ADC value per motor
};

struct EncoderData {
    std::array<float, NUM_MOTORS> position;
    std::array<float, NUM_MOTORS> velocity;
};
