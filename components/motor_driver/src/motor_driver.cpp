#include "motor_driver.hpp"

extern "C"
{
#include "esp_log.h"
#include "esp_check.h"
#include "driver/mcpwm_cap.h"

#include <cstring>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/rmt_rx.h"
}

#include "pin_config.h"
#include <algorithm>
#include <cmath>
static const char *TAG = "MotorDriver";

MotorDriver::MotorDriver(const char* motor_name,
                                int mcpwm_unit,
                                gpio_num_t pwm_high_gpio,
                                gpio_num_t pwm_low_gpio,
                                gpio_num_t encoder_gpio)
    : motor_name_(motor_name),
      mcpwm_unit_(mcpwm_unit),
      pwm_high_gpio_(pwm_high_gpio),
      pwm_low_gpio_(pwm_low_gpio),
      encoder_gpio_(encoder_gpio) {}

esp_err_t MotorDriver::init()
{
    if (initialized_)
    {
        ESP_LOGW(TAG, "MotorDriver already initialized");
        return ESP_OK;
    }

    // MCPWM timer config
    mcpwm_timer_config_t timer_config = {
        .group_id = mcpwm_unit_,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 4000000, // 4 MHz
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = 1000, // 4 kHz PWM, 1 tick = 0.25 us
        .intr_priority = 0};
    ESP_RETURN_ON_ERROR(mcpwm_new_timer(&timer_config, &timer_), TAG, "Failed to create timer");

    // MCPWM operator
    mcpwm_operator_config_t operator_config = {};
    operator_config.group_id = mcpwm_unit_;
    ESP_RETURN_ON_ERROR(mcpwm_new_operator(&operator_config, &operator_), TAG, "Failed to create operator");

    // Connect operator to timer
    ESP_RETURN_ON_ERROR(mcpwm_operator_connect_timer(operator_, timer_), TAG, "Failed to connect operator to timer");

    // Create comparators
    mcpwm_comparator_config_t comparator_config = {};
    comparator_config.flags.update_cmp_on_tez = true;
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(operator_, &comparator_config, &comparator_high_), TAG, "Failed to create comparator high");
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(operator_, &comparator_config, &comparator_low_), TAG, "Failed to create comparator low");

    // Set initial compare values
    mcpwm_comparator_set_compare_value(comparator_high_, 0);
    mcpwm_comparator_set_compare_value(comparator_low_, 0);

    // Create generators
    mcpwm_generator_config_t gen_high_cfg = {};
    gen_high_cfg.gen_gpio_num = pwm_high_gpio_;
    mcpwm_generator_config_t gen_low_cfg = {};
    gen_low_cfg.gen_gpio_num = pwm_low_gpio_;
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(operator_, &gen_high_cfg, &generator_high_), TAG, "Failed to create generator high");
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(operator_, &gen_low_cfg, &generator_low_), TAG, "Failed to create generator low");

    // Set generator actions for high side
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
        generator_high_,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END()));
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
        generator_high_,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_high_, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()));

    // Set generator actions for low side
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
        generator_low_,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END()));
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
        generator_low_,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_low_, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()));

    // Enable and start timer
    ESP_RETURN_ON_ERROR(mcpwm_timer_enable(timer_), TAG, "Failed to enable timer");
    ESP_RETURN_ON_ERROR(mcpwm_timer_start_stop(timer_, MCPWM_TIMER_START_NO_STOP), TAG, "Failed to start timer");

    ESP_RETURN_ON_ERROR(init_encoder(), TAG, "Failed to initialize encoder");

    initialized_ = true;

    disarm_motor();
    return ESP_OK;
}


esp_err_t MotorDriver::disarm_motor()
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Stop the position control task
    if (control_task_handle_ != nullptr)
    {
        vTaskDelete(control_task_handle_);
        control_task_handle_ = nullptr;
    }
    // Set motor speed to 0 when disarming
    this->setSpeed(0.0f);

    return ESP_OK;
}

esp_err_t MotorDriver::setSpeed(float speed)
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (speed < -1.0f)
        speed = -1.0f;
    if (speed > 1.0f)
        speed = 1.0f;

    const uint32_t duty_ticks = static_cast<uint32_t>((speed < 0 ? -speed : speed) * 1000); // 1000 ticks = 100% duty (25kHz)

    if (speed >= 0)
    {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, duty_ticks), TAG, "Set comp high");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, 0), TAG, "Set comp low to 0");
    }
    else
    {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, 0), TAG, "Set comp high to 0");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, duty_ticks), TAG, "Set comp low");
    }
    pwm_speed_ = speed;

    return ESP_OK;
}

