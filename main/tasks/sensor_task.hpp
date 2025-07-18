#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
}

#include <array>
#include "sensor_driver/imu.hpp"   // Stub: IMU driver via I2C
#include "sensor_driver/adc.hpp"   // Stub: ADC driver via SPI
#include "sensor_driver/encoder.hpp" // Stub: Encoder driver via input capture

#define NUM_MOTORS 5

struct SensorData {
    std::array<float, 3> imu_data;      // e.g., [accel_x, accel_y, accel_z]
    std::array<float, NUM_MOTORS> adc_data; // e.g., one ADC value per motor
    std::array<float, NUM_MOTORS> encoder_position;
    std::array<float, NUM_MOTORS> encoder_velocity;
};

class SensorTask {
public:
    SensorTask(const char* name, uint32_t stackSize, UBaseType_t priority)
        : taskName(name), stackSize(stackSize), priority(priority) {
        dataMutex = xSemaphoreCreateMutex();
    }

    void start() {
        xTaskCreate(&SensorTask::taskFunction, taskName, stackSize, this, priority, nullptr);
    }

    // Thread-safe getter for sensor data
    bool getSensorData(SensorData& outData) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            outData = sensorData;
            xSemaphoreGive(dataMutex);
            return true;
        }
        return false;
    }

private:
    static void taskFunction(void* pvParameters) {
        SensorTask* self = static_cast<SensorTask*>(pvParameters);
        IMU imu;
        ADC adc;
        Encoder encoder;
        while (true) {
            SensorData newData;
            // Read IMU data via I2C
            newData.imu_data = imu.read();
            // Read ADC data via SPI
            for (int i = 0; i < NUM_MOTORS; ++i) {
                newData.adc_data[i] = adc.read(i);
            }
            // Read encoder data via input capture
            for (int i = 0; i < NUM_MOTORS; ++i) {
                newData.encoder_position[i] = encoder.getPosition(i);
                newData.encoder_velocity[i] = encoder.getVelocity(i);
            }
            // Store data thread-safely
            if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                self->sensorData = newData;
                xSemaphoreGive(self->dataMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(5)); // 200 Hz sensor update
        }
    }

    SensorData sensorData;
    SemaphoreHandle_t dataMutex;
    const char* taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};
