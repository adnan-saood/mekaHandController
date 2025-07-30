#pragma once
#include "config.h"
extern "C" {
#include "driver/gpio.h"
}
    
class MotorDriver {
public:
    MotorDriver(gpio_num_t pwmA, gpio_num_t pwmB);
    void init();
    void setPWM(float duty_cycle);  // duty_cycle in [-1.0, 1.0]

private:
    // Private member variables for motor control
    int pwm_channel;
    float current_duty_cycle;
    gpio_num_t gpio_pwmA; // GPIO pin for PWM A
    gpio_num_t gpio_pwmB; // GPIO pin for PWM B
};