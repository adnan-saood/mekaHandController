// MotorDriver.hpp

#ifndef MOTOR_DRIVER_HPP
#define MOTOR_DRIVER_HPP

extern "C" {
#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"
#include "esp_err.h"
}

class MotorDriver {
public:
    /**
     * @brief Construct a new MotorDriver instance.
     * 
     * @param mcpwm_unit MCPWM unit (0 or 1 on ESP32-S3)
     * @param pwm_high_gpio GPIO for high-side PWM (gpio_num_t)
     * @param pwm_low_gpio GPIO for low-side PWM (gpio_num_t)
     */
    MotorDriver(int mcpwm_unit, gpio_num_t pwm_high_gpio, gpio_num_t pwm_low_gpio);

    /**
     * @brief Initialize the motor driver (MCPWM timer, operator, comparators, generators).
     *        Must be called before using setSpeed().
     * 
     * @return esp_err_t ESP_OK on success, or error code.
     */
    esp_err_t init();

    /**
     * @brief Set motor speed.
     * 
     * @param speed Motor speed in range [-1.0, 1.0]. Negative = reverse, Positive = forward.
     * @return esp_err_t ESP_OK on success, or error code.
     */
    esp_err_t setSpeed(float speed);

private:
    const int mcpwm_unit_;
    const gpio_num_t pwm_high_gpio_;
    const gpio_num_t pwm_low_gpio_;

    mcpwm_timer_handle_t timer_ = nullptr;
    mcpwm_oper_handle_t operator_ = nullptr;
    mcpwm_cmpr_handle_t comparator_high_ = nullptr;
    mcpwm_cmpr_handle_t comparator_low_ = nullptr;
    mcpwm_gen_handle_t generator_high_ = nullptr;
    mcpwm_gen_handle_t generator_low_ = nullptr;

    bool initialized_ = false;

};

#endif // MOTOR_DRIVER_HPP
