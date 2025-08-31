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
    const TickType_t xPeriod = pdMS_TO_TICKS(10); // 10 ms
    const float Ts = 0.01f;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // tuning starting points (tweak)
    // driver->kp = 0.8f;
    // driver->ki = 0.2f;   // small because motor integrates speed->position
    // driver->kd = 0.0f;   // usually 0 for outer position loop

    const float min_effective_speed = 0.12f; // minimal speed to reliably overcome stiction
    const float position_deadband = 0.003f;  // if position error smaller than this, treat as zero
    const float i_max = 0.5f;                // integrator clamp (tune)
    const float i_min = -0.5f;

    while (true) {
        // read errors
        float pos = driver->getPosition();
        float err = driver->commanded_position_ - pos;

        // optional position deadband to avoid hunting around tiny errors
        if (fabs(err) < position_deadband) {
            err = 0.0f;
        }

        // ANTI-WINDUP: conditional integration (only integrate if output not saturated)
        // compute proportional contribution
        float up = driver->kp * err;

        // decide whether to integrate: integrate only if error is meaningful
        bool integrate = fabs(err) > (position_deadband * 0.5f);

        if (integrate) {
            driver->integral_error_ += err * Ts; // scale by sample time
        } else {
            // optional: slow leak when inside deadband to remove residual offset
            driver->integral_error_ *= 0.999f; // leaky integrator
        }

        // clamp integrator (anti-windup)
        if (driver->integral_error_ > i_max) driver->integral_error_ = i_max;
        if (driver->integral_error_ < i_min) driver->integral_error_ = i_min;

        float ui = driver->ki * driver->integral_error_;

        // optional derivative (be careful: noisy)
        float derivative = (err - driver->proportional_error_) / Ts;
        driver->proportional_error_ = err;
        float ud = driver->kd * derivative;

        // assemble control (this is a position->speed command)
        float speed_cmd = up + ui + ud;

        // --- anti-windup back-calculation (optional) ---
        // If you clamp the final speed_cmd below, you can push back on the integrator:
        // float raw = speed_cmd;
        // speed_cmd = std::clamp(speed_cmd, -1.0f, 1.0f);
        // float saturated = speed_cmd - raw;
        // driver->integral_error_ += -sat_gain * saturated; // sat_gain ~ 1/Ti (tune)
        // For simplicity we use clamping + conditional integration above.

        // minimum effective speed instead of hard zero deadzone
        if (fabs(speed_cmd) > 0.0f && fabs(speed_cmd) < min_effective_speed) {
            // only boost to min if error indicates we actually want to move
            speed_cmd = copysign(min_effective_speed, speed_cmd);
        }

        // final clamp
        speed_cmd = std::clamp(speed_cmd, -1.0f, 1.0f);

        driver->setSpeed(speed_cmd);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}
