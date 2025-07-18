#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include "motor_driver.hpp"
class PIDFFController {
public:
    PIDFFController();
    float compute(float setpoint, float position, float velocity);

private:
    float kp, ki, kd, kff;
    float integral, prev_error;
};

class MotorTask {
public:
    MotorTask(const char* name = "Motor", uint32_t stackSize = 4096, UBaseType_t priority = 3);
    void start();

private:
    static void taskFunction(void* pvParameters);
    static void controlLoop();

    static PIDFFController controller;
    const char* taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};
