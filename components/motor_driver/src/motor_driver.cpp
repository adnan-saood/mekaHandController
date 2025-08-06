// MotorDriver.cpp

#include "MotorDriver.hpp"
#include "esp_log.h"

static const char* TAG = "MotorDriver";

MotorDriver::MotorDriver(int mcpwm_unit, gpio_num_t pwm_high_gpio, gpio_num_t pwm_low_gpio)
    : mcpwm_unit_(mcpwm_unit),
      pwm_high_gpio_(pwm_high_gpio),
      pwm_low_gpio_(pwm_low_gpio) {}

esp_err_t MotorDriver::init() {
    if (initialized_) {
        ESP_LOGW(TAG, "MotorDriver already initialized");
        return ESP_OK;
    }

    // MCPWM timer config
    mcpwm_timer_config_t timer_config = {
        .group_id = mcpwm_unit_,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 4e6, // 4 MHz
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = 1000, // 4 kHz PWM 1 tick = 0.25 us
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_timer(&timer_config, &timer_), TAG, "Failed to create timer");

    // MCPWM operator
    mcpwm_operator_config_t operator_config = {
        .group_id = mcpwm_unit_,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_operator(&operator_config, &operator_), TAG, "Failed to create operator");

    // Connect operator to timer
    ESP_RETURN_ON_ERROR(mcpwm_operator_connect_timer(operator_, timer_), TAG, "Failed to connect operator to timer");

    // Create comparators
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(operator_, &comparator_config, &comparator_high_), TAG, "Failed to create comparator high");
    ESP_RETURN_ON_ERROR(mcpwm_new_comparator(operator_, &comparator_config, &comparator_low_), TAG, "Failed to create comparator low");

    // Create generators
    mcpwm_generator_config_t gen_high_cfg = {
        .gen_gpio_num = pwm_high_gpio_
    };
    mcpwm_generator_config_t gen_low_cfg = {
        .gen_gpio_num = pwm_low_gpio_
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(operator_, &gen_high_cfg, &generator_high_), TAG, "Failed to create generator high");
    ESP_RETURN_ON_ERROR(mcpwm_new_generator(operator_, &gen_low_cfg, &generator_low_), TAG, "Failed to create generator low");

    // Set generator actions for high side
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_action_on_timer_event(generator_high_,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)),
        TAG, "Failed to set generator high action on TEZ");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_action_on_compare_event(generator_high_, comparator_high_,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_GEN_ACTION_LOW)),
        TAG, "Failed to set generator high action on compare");

    // Set generator actions for low side
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_action_on_timer_event(generator_low_,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)),
        TAG, "Failed to set generator low action on TEZ");
    ESP_RETURN_ON_ERROR(mcpwm_generator_set_action_on_compare_event(generator_low_, comparator_low_,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_GEN_ACTION_LOW)),
        TAG, "Failed to set generator low action on compare");

    // Enable and start timer
    ESP_RETURN_ON_ERROR(mcpwm_timer_enable(timer_), TAG, "Failed to enable timer");
    ESP_RETURN_ON_ERROR(mcpwm_timer_start_stop(timer_, MCPWM_TIMER_START_NO_STOP), TAG, "Failed to start timer");

    initialized_ = true;
    return ESP_OK;
}

esp_err_t MotorDriver::setSpeed(float speed) {
    if (!initialized_) {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (speed < -1.0f) speed = -1.0f;
    if (speed >  1.0f) speed =  1.0f;

    return updatePwm(speed);
}

esp_err_t MotorDriver::updatePwm(float speed) {
    const uint32_t duty_ticks = static_cast<uint32_t>(fabs(speed) * 400); // 400 ticks = 100% duty (25kHz)

    if (speed >= 0) {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, duty_ticks), TAG, "Set comp high");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, 0), TAG, "Set comp low to 0");
    } else {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, 0), TAG, "Set comp high to 0");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, duty_ticks), TAG, "Set comp low");
    }

    return ESP_OK;
}