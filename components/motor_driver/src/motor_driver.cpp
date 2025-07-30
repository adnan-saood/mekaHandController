#include "motor_driver.hpp"
#include <cstdio> // Replace with real driver includes for PWM

extern "C"
{
#include "driver/ledc.h"
}

MotorDriver::MotorDriver(gpio_num_t pwmA,
    uint8_t pwmA_channel,
    gpio_num_t pwmB,
    uint8_t pwmB_channel)
{
    this->gpio_pwmA = pwmA;
    this->gpio_pwmB = pwmB;
    this->pwmA_channel_num = pwmA_channel;
    this->pwmB_channel_num = pwmB_channel;

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

void MotorDriver::init()
{
    // Prepare and then apply the LEDC channel configuration
    ledc_channel_config_t pwmA_channel = {
        .gpio_num = this->gpio_pwmA,
        .speed_mode = LEDC_MODE,
        .channel = static_cast<ledc_channel_t>(this->pwmA_channel_num),
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Set duty to 0%
        .hpoint = 0,
        .flags = {0}
    };

    ledc_channel_config_t pwmB_channel = {
        .gpio_num = this->gpio_pwmB,
        .speed_mode = LEDC_MODE,
        .channel = static_cast<ledc_channel_t>(this->pwmB_channel_num),
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Set duty to 0%
        .hpoint = 0,
        .flags = {0}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&pwmA_channel));
    ESP_ERROR_CHECK(ledc_channel_config(&pwmB_channel));
}

void MotorDriver::setPWM(float duty_cycle)
{
    // Clamp value
    if (duty_cycle > 1.0f)
        duty_cycle = 1.0f;
    if (duty_cycle < -1.0f)
        duty_cycle = -1.0f;

    // Convert to hardware format and write
    printf("[PWM] Writing duty cycle: %.2f\n", duty_cycle);

    // Convert to hardware format and write
    uint16_t pwm_A = (duty_cycle > 0.0f) ? uint16_t(duty_cycle * 8191) : 0;
    uint16_t pwm_B = (duty_cycle < 0.0f) ? uint16_t(-duty_cycle * 8191) : 0;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, pwm_A));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, pwm_B));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
}
