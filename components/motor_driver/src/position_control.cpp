#include "motor_driver.hpp"

extern "C"
{
#include "esp_log.h"
#include "esp_check.h"
#include "driver/mcpwm_cap.h"

#include <cstring>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/rmt_rx.h"
}

#include "pin_config.h"
#include <algorithm>
#include <cmath>
static const char *TAG = "PositionControl";



esp_err_t MotorDriver::startPositionControl()
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    char control_task_name[32];
    snprintf(control_task_name, sizeof(control_task_name), "%s_control", motor_name_);
    // Create control task
    xTaskCreate(control_loop, control_task_name, 4096, this, 10, &(this->control_task_handle_));
    return ESP_OK;
}

esp_err_t MotorDriver::stopPositionControl()
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Stop the position control task
    if (control_task_handle_ != nullptr)
    {
        vTaskDelete(control_task_handle_);
        control_task_handle_ = nullptr;
    }

    return this->disarm_motor();
}




extern "C" void MotorDriver::control_loop(void* pvParameters)
{
    MotorDriver* driver = static_cast<MotorDriver*>(pvParameters);
    const TickType_t xPeriod = pdMS_TO_TICKS(10);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // Implement your control logic here
    while (true) {
        float p_error = driver->commanded_position_ - driver->getPosition();
        driver->derivative_error_ = p_error - driver->proportional_error_;
        driver->proportional_error_ = p_error;
        float integral_error = driver->integral_error_ += p_error;
        //clamp integral error
        if (integral_error > 1.0f) integral_error = 1.0f;
        if (integral_error < -1.0f) integral_error = -1.0f;
        float control_signal = driver->kp * p_error + driver->ki * integral_error + driver->kd * driver->derivative_error_;
    
        // implement deadzone
        if (fabs(control_signal) < 0.05f) {
            control_signal = 0.0f;
        }

        driver->setSpeed(control_signal);
        // ESP_LOGI(TAG, "Position: %.2f, error: %.2f, control_signal: %.2f", driver->getPosition(), p_error, control_signal);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}