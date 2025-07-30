#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // This header defines SemaphoreHandle_t
}

#include <array>
#include "imu.hpp"   // Stub: IMU driver via I2C
#include "adc.hpp"   // Stub: ADC driver via SPI
#include "encoder.hpp" // Stub: Encoder driver via input capture

#include "config.h"
#include "sensors.hpp" // SensorData struct



class SensorTask {
public:
    SensorTask(uint32_t stackSize, UBaseType_t priority)
        : stackSize(stackSize), priority(priority) {
        dataMutex = xSemaphoreCreateMutex();
    }

    void start();

    // Thread-safe getter for sensor data
    bool getIMUData(IMUData& outData);
    bool getADCData(ADCData& outData);
    bool getEncoderData(EncoderData& outData);

private:

    static void imuTask(void* pvParameters);
    static void adcTask(void* pvParameters);
    static void encoderTask(void* pvParameters);

    IMUData imu_data;
    ADCData adc_data;
    EncoderData encoder_data;
    SemaphoreHandle_t dataMutex;
    uint32_t stackSize;
    UBaseType_t priority;
};
