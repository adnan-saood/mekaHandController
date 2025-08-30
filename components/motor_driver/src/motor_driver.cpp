#include "motor_driver.hpp"

extern "C"
{
#include "esp_log.h"
#include "esp_check.h"
#include "driver/mcpwm_cap.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/rmt_rx.h"
}

#include "pin_config.h"
static const char *TAG = "MotorDriver";
static const char *BLDC_TAG = "MotorDriverBLDC";

// Task handle for encoder reading
static TaskHandle_t encoder_task_handle = nullptr;
volatile int ma3_position = 0; // Shared variable for position

MotorDriver::MotorDriver(int mcpwm_unit, gpio_num_t pwm_high_gpio, gpio_num_t pwm_low_gpio, gpio_num_t encoder_gpio)
    : mcpwm_unit_(mcpwm_unit),
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
    return ESP_OK;
}

esp_err_t MotorDriver::init_encoder()
{
    ESP_LOGI(TAG, "Install capture timer");
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .group_id = mcpwm_unit_,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .resolution_hz = 80000000 // 80 MHz
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = GPIO_NUM_14,
        // .intr_priority = 1,
        .prescale = 1,
        // flags will be set below
    };
    cap_ch_conf.flags.pos_edge = true;
    cap_ch_conf.flags.neg_edge = true;
    cap_ch_conf.flags.pull_up = false;
    cap_ch_conf.flags.pull_down = true;
    cap_ch_conf.flags.invert_cap_signal = false;

    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = encoder_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, this));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));

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

    return ESP_OK;
}

esp_err_t MotorDriverBLDC::setSpeed(float speed)
{
    if (!initialized_)
    {
        ESP_LOGE(TAG, "MotorDriver not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    // Ensure speed is within [-1.0, 1.0]
    if (speed < -1.0f)
        speed = -1.0f;
    if (speed > 1.0f)
        speed = 1.0f;

    const uint32_t duty_ticks = static_cast<uint32_t>((speed < 0 ? -speed : speed) * 1000); // 1000 ticks = 100% duty (25kHz)

    if (speed >= 0)
    {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, duty_ticks), BLDC_TAG, "Set Speed");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, 0), BLDC_TAG, "Set Direction");
    }
    else
    {
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_high_, duty_ticks), BLDC_TAG, "Set Speed");
        ESP_RETURN_ON_ERROR(mcpwm_comparator_set_compare_value(comparator_low_, 1000), BLDC_TAG, "Set Direction");
    }
    return ESP_OK;
}

IRAM_ATTR bool MotorDriver::encoder_callback(mcpwm_cap_channel_handle_t cap_chan,
                             const mcpwm_capture_event_data_t *edata,
                             void *user_data)
{
    static uint32_t cap_val_rise = 0;
    MotorDriver* self = reinterpret_cast<MotorDriver*>(user_data);

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS)
    {
        cap_val_rise = edata->cap_value;
    }
    else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG)
    {
        uint32_t pulse_width = edata->cap_value - cap_val_rise;
        if (self) {
            self->ma3_pulse_width_ = pulse_width;
        }
    }
    return false;
}

uint32_t MotorDriver::getEncoderPosition() const
{
    return ma3_pulse_width_ / 80.0; // add calculation for position
}