#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
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

    void start() {
        xTaskCreate(&SensorTask::imuTask, "IMU_Task", stackSize, this, priority, nullptr);
        xTaskCreate(&SensorTask::adcTask, "ADC_Task", stackSize, this, priority, nullptr);
        xTaskCreate(&SensorTask::encoderTask, "Encoder_Task", stackSize, this, priority, nullptr);
    }

    // Thread-safe getter for sensor data
    bool getIMUData(IMUData& outData) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            outData = imu_data;
            xSemaphoreGive(dataMutex);
            return true;
        }
        return false;
    }

    bool getADCData(ADCData& outData) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            outData = adc_data;
            xSemaphoreGive(dataMutex);
            return true;
        }
        return false;
    }

    bool getEncoderData(EncoderData& outData) {
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            outData = encoder_data;
            xSemaphoreGive(dataMutex);
            return true;
        }
        return false;
    }

private:
    static void imuTask(void* pvParameters) {
        SensorTask* self = static_cast<SensorTask*>(pvParameters);
        IMU imu;
        while (true) {
            IMUData newIMUData = imu.read();
            if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                self->imu_data = newIMUData;
                xSemaphoreGive(self->dataMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
        }
    }

    static void adcTask(void* pvParameters) {
        SensorTask* self = static_cast<SensorTask*>(pvParameters);
        ADC adc;
        while (true) {
            ADCData newADCData;
            for (int i = 0; i < NUM_MOTORS; ++i) {
                newADCData.values[i] = adc.read(i);
            }
            if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                self->adc_data = newADCData;
                xSemaphoreGive(self->dataMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
        }
    }

    static void encoderTask(void* pvParameters) {
        SensorTask* self = static_cast<SensorTask*>(pvParameters);
        Encoder encoder;
        while (true) {
            EncoderData newEncoderData;
            for (int i = 0; i < NUM_MOTORS; ++i) {
                newEncoderData.position[i] = encoder.getPosition(i);
                newEncoderData.velocity[i] = encoder.getVelocity(i);
            }
            if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                self->encoder_data = newEncoderData;
                xSemaphoreGive(self->dataMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
        }
    }

    IMUData imu_data;
    ADCData adc_data;
    EncoderData encoder_data;
    SemaphoreHandle_t dataMutex;
    uint32_t stackSize;
    UBaseType_t priority;
};
