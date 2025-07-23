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
    void start();

private:
    static void taskFunction(void *pvParameters);

    static void controlLoop();

    static PIDFFController controllers[NUM_MOTORS];
    const char *taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};