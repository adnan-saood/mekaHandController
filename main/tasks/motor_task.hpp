#pragma once

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#define NUM_MOTORS 5

#include "motor_driver.hpp"

class PIDFFController
{
public:
    PIDFFController()
        : kp(1.0f), ki(0.0f), kd(0.0f), kff(0.0f), integral(0.0f), prev_error(0.0f) {}

    float compute(float setpoint, float position, float velocity)
    {
        float error = setpoint - position;
        integral += error;
        float derivative = error - prev_error;
        prev_error = error;

        float output = kp * error + ki * integral + kd * derivative + kff * velocity;
        return output;
    }

private:
    float kp, ki, kd, kff;
    float integral, prev_error;
};

class MotorTask
{
public:
    MotorTask(const char *name, uint32_t stackSize, UBaseType_t priority)
        : taskName(name), stackSize(stackSize), priority(priority) {}
    void start()
    {
        xTaskCreate(&MotorTask::taskFunction, taskName, stackSize, nullptr, priority, nullptr);
    }

private:
    static void taskFunction(void *pvParameters)
    {
        while (true)
        {
            controlLoop();
            vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz control loop
        }
    }

    static void controlLoop()
    {
        for (int i = 0; i < NUM_MOTORS; ++i)
        {
            float desired_position = 1.0f; // Stub: get from USB or initial value per motor
            float current_position = 0.5f; // Stub: get from encoder per motor
            float current_velocity = 0.1f; // Stub: optional if controller supports it per motor

            float output_pwm = controllers[i].compute(desired_position, current_position, current_velocity);

            // Stub: send PWM signal to motor driver here
            printf("[Motor %d] PWM Output: %.2f\n", i, output_pwm);
        }
    }

    static PIDFFController controllers[NUM_MOTORS];
    const char *taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};