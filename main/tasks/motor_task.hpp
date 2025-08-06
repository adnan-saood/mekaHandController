#pragma once

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}
#include "pin_config.h"
#include "usb_driver.hpp"

#define NUM_MOTORS 5

#include "motor_driver.hpp"

class PIDFFController
{
public:
    PIDFFController()
        : kp(1.0f), ki(0.0f), kd(0.0f), kff(0.0f), integral(0.0f), prev_error(0.0f) {}

    PIDFFController(float kp, float ki, float kd, float kff)
        : kp(kp), ki(ki), kd(kd), kff(kff), integral(0.0f), prev_error(0.0f) {} 

    void setGains(float kp, float ki, float kd, float kff)
    {
        this->kp = kp;
        this->ki = ki;
        this->kd = kd;
        this->kff = kff;
    }

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

    void initMotorDrivers();
    void setUsbHandle(USBDriver *usbDriver) { MotorTask::usbDriver = usbDriver; }
    void start();

private:
    static void taskFunction(void *pvParameters);

    static void controlLoop();

    static PIDFFController thumbPanController;
    static PIDFFController thumbController;
    static PIDFFController indexController;
    static PIDFFController middleController;
    static PIDFFController pinkyController;

    static MotorDriverBLDC* thumbPanMotor;
    static MotorDriverBLDC* thumbMotor;
    static MotorDriverBLDC* indexMotor;
    static MotorDriverBLDC* middleMotor;
    static MotorDriverBLDC* pinkyMotor;

    // usb communication handle
    static USBDriver* usbDriver = nullptr;


    static std::array<MotorDriver*, NUM_MOTORS> motor;
    static std::array<PIDFFController*, NUM_MOTORS> controllers;

    const char *taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};