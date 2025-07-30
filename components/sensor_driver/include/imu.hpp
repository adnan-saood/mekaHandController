#pragma once
#include "sensors.hpp"

class IMU {
public:
    IMUData read() {
        IMUData data;
        data.accel = {0.0f, 0.0f, 0.0f}; // Replace with actual reading logic
        data.gyro = {0.0f, 0.0f, 0.0f};  // Replace with actual reading logic
        
        // Read IMU data via I2C
        return data;
    }
};