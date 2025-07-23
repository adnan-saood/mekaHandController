#include "sensor_task.hpp"

void SensorTask::start()
{
    xTaskCreate(&SensorTask::imuTask, "IMU_Task", stackSize, this, priority, nullptr);
    xTaskCreate(&SensorTask::adcTask, "ADC_Task", stackSize, this, priority, nullptr);
    xTaskCreate(&SensorTask::encoderTask, "Encoder_Task", stackSize, this, priority, nullptr);
}

bool SensorTask::getIMUData(IMUData &outData)
{
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
    {
        outData = imu_data;
        xSemaphoreGive(dataMutex);
        return true;
    }
    return false;
}

bool SensorTask::getADCData(ADCData &outData)
{
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
    {
        outData = adc_data;
        xSemaphoreGive(dataMutex);
        return true;
    }
    return false;
}

bool SensorTask::getEncoderData(EncoderData &outData)
{
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
    {
        outData = encoder_data;
        xSemaphoreGive(dataMutex);
        return true;
    }
    return false;
}

void SensorTask::imuTask(void *pvParameters)
{
    SensorTask *self = static_cast<SensorTask *>(pvParameters);
    IMU imu;
    while (true)
    {
        IMUData newIMUData = imu.read();
        if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            self->imu_data = newIMUData;
            xSemaphoreGive(self->dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
    }
}

void SensorTask::adcTask(void *pvParameters)
{
    SensorTask *self = static_cast<SensorTask *>(pvParameters);
    ADC adc;
    while (true)
    {
        // This ADCData hold all ADC values and gets distributed later.
        ADCData newADCData = adc.read();

        if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            self->adc_data = newADCData;
            xSemaphoreGive(self->dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
    }

}

void SensorTask::encoderTask(void *pvParameters)
{
    SensorTask *self = static_cast<SensorTask *>(pvParameters);
    Encoder encoder;
    while (true)
    {
        // This EncoderData hold all encoder values and gets distributed later.
        EncoderData newEncoderData = encoder.read();
        if (xSemaphoreTake(self->dataMutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            self->encoder_data = newEncoderData;
            xSemaphoreGive(self->dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz
    }
}