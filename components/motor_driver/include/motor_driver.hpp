#pragma once
#include "config.h"
extern "C" {
#include "driver/gpio.h"
}

class MotorDriver {
public:
    MotorDriver(gpio_num_t pwmA, uint8_t pwmA_channel,
                gpio_num_t pwmB, uint8_t pwmB_channel);
    void init();
    void setPWM(float duty_cycle);  // duty_cycle in [-1.0, 1.0]

private:
    // Private member variables for motor control
    int pwm_channel;
    float current_duty_cycle;
    gpio_num_t gpio_pwmA; // GPIO pin for PWM A
    gpio_num_t gpio_pwmB; // GPIO pin for PWM B
    uint8_t pwmA_channel_num;
    uint8_t pwmB_channel_num;
};