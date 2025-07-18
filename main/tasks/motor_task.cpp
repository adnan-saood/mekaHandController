#include "motor_task.hpp"
#include <cstdio>


PIDFFController::PIDFFController()
    : kp(1.0f), ki(0.0f), kd(0.0f), kff(0.0f), integral(0.0f), prev_error(0.0f) {}

float PIDFFController::compute(float setpoint, float position, float velocity) {
    float error = setpoint - position;
    integral += error;
    float derivative = error - prev_error;
    prev_error = error;

    float output = kp * error + ki * integral + kd * derivative + kff * velocity;
    return output;
}

// Static controller instance
PIDFFController MotorTask::controller;

MotorTask::MotorTask(const char* name, uint32_t stackSize, UBaseType_t priority)
    : taskName(name), stackSize(stackSize), priority(priority) {}

void MotorTask::start() {
    xTaskCreate(&MotorTask::taskFunction, taskName, stackSize, nullptr, priority, nullptr);
}

void MotorTask::taskFunction(void* pvParameters) {
    while (true) {
        controlLoop();
        vTaskDelay(pdMS_TO_TICKS(10));  // 100 Hz control loop
    }
}

void MotorTask::controlLoop() {
    float desired_position = 1.0f;  // Stub: get from config or USB
    float current_position = 0.5f;  // Stub: get from encoder
    float current_velocity = 0.1f;  // Stub: optional if controller supports it

    float output_pwm = controller.compute(desired_position, current_position, current_velocity);

    // Stub: send PWM signal to motor driver here
    printf("[Motor] PWM Output: %.2f\n", output_pwm);
}
