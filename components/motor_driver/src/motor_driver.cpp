#include "motor_driver.hpp"
#include "driver/mcpwm_prelude.h"
#include <cstdio>

MotorDriver::MotorDriver(gpio_num_t pwmA, gpio_num_t pwmB, int mcpwm_group)
    : gpio_pwmA(pwmA),
      gpio_pwmB(pwmB),
      mcpwm_group(mcpwm_group),
      timer_handle(nullptr),
      opA_handle(nullptr),
      opB_handle(nullptr),
      pwmA_cmpr(nullptr),
      pwmB_cmpr(nullptr),
      genA_handle(nullptr),
      genB_handle(nullptr)
{
}

void MotorDriver::init()
{
   
}

void MotorDriver::setSpeed(float speed)
{
    // Clamp speed between -1.0 and 1.0
    if (speed < -1.0f) speed = -1.0f;
    if (speed > 1.0f) speed = 1.0f;

    // Set PWM duty cycle based on speed
    float dutyA = (speed >= 0) ? speed : 0;
    float dutyB = (speed >= 0) ? 0 : -speed;

    uint32_t period = 400; // Must match period_ticks in init()
    uint32_t cmpA = static_cast<uint32_t>(dutyA * period);
    uint32_t cmpB = static_cast<uint32_t>(dutyB * period);

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(pwmA_cmpr, cmpA));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(pwmB_cmpr, cmpB));
}
