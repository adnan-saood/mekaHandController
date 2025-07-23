
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h" // Add this include for ESP_LOGI

#include "tasks/logger_task.hpp"
#include "tasks/motor_task.hpp"
#include "tasks/sensor_task.hpp"


extern "C" void app_main(void)
{
    //No operation.
    printf("Motor Control Example\n");

    // LoggerTask loggerTask;
    // loggerTask.start();


    // Add a motortask
    // MotorTask motorTask("Motor_Task", 4096, 2);
    // motorTask.start();


    SensorTask sensorTask(2048, 2);
    sensorTask.start();

    while(1)
    {
        // Main loop code
        vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
        IMUData imuData;
        if (sensorTask.getIMUData(imuData))
        {
            // log IMU data using esp log directives
            ESP_LOGI("IMU", "Accel: [%f, %f, %f], Gyro: [%f, %f, %f]",
                     imuData.accel[0], imuData.accel[1], imuData.accel[2],
                     imuData.gyro[0], imuData.gyro[1], imuData.gyro[2]);
            
        }
        ADCData adcData;
        if (sensorTask.getADCData(adcData))
        {
            // log ADC data using esp log directives
            ESP_LOGI("ADC", "Values: [%f, %f, %f, %f, %f]",
                     adcData.values[0], adcData.values[1], adcData.values[2],
                     adcData.values[3], adcData.values[4]);
        }
        EncoderData encoderData;
        if (sensorTask.getEncoderData(encoderData))
        {
            // log Encoder data using esp log directives
            ESP_LOGI("Encoder", "Position: %f, Velocity: %f",
                     encoderData.position[0], encoderData.velocity[0]);
        }
    }
}