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


static const char *TAG = "EncoderDriver";

static mcpwm_cap_timer_handle_t shared_cap_timer[2] = {nullptr, nullptr}; // For group 0 and 1


esp_err_t MotorDriver::init_encoder()
{
    ESP_LOGI(TAG, "Install capture timer");

    // Only create the timer once per group
    if (shared_cap_timer[mcpwm_unit_] == nullptr) {
        mcpwm_capture_timer_config_t cap_conf = {
            .group_id = mcpwm_unit_,
            .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
            .resolution_hz = 80000000 // 80 MHz
        };
        ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &shared_cap_timer[mcpwm_unit_]));
        ESP_ERROR_CHECK(mcpwm_capture_timer_enable(shared_cap_timer[mcpwm_unit_]));
        ESP_ERROR_CHECK(mcpwm_capture_timer_start(shared_cap_timer[mcpwm_unit_]));
    }

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = encoder_gpio_,
        .prescale = 1,
    };
    cap_ch_conf.flags.pos_edge = true;
    cap_ch_conf.flags.neg_edge = true;
    cap_ch_conf.flags.pull_up = false;
    cap_ch_conf.flags.pull_down = true;
    cap_ch_conf.flags.invert_cap_signal = false;

    ESP_ERROR_CHECK(mcpwm_new_capture_channel(shared_cap_timer[mcpwm_unit_], &cap_ch_conf, &cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = encoder_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, this));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    return ESP_OK;
}

esp_err_t MotorDriver::setEncoderLimits(float min, float max)
{
    if (min >= max)
    {
        return ESP_ERR_INVALID_ARG;
    }

    this->open_position_ = min;
    this->closed_position_ = max;
    return ESP_OK;
}

IRAM_ATTR bool MotorDriver::encoder_callback(mcpwm_cap_channel_handle_t cap_chan,
                             const mcpwm_capture_event_data_t *edata,
                             void *user_data)
{
    MotorDriver* self = reinterpret_cast<MotorDriver*>(user_data);

    if (!self)
    {
        return false;
    }

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS)
    {
        self->cap_val_rise_ = edata->cap_value;
    }
    else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG)
    {
        uint32_t pulse_width = (edata->cap_value - self->cap_val_rise_) / 80;
        // Insert new value at current index
        self->pulse_array_[self->pulse_index_] = pulse_width;
        self->pulse_index_ = (self->pulse_index_ + 1) % 11; // Circular buffer

        // Copy to temp array for sorting
        uint32_t temp_array[11];
        memcpy(temp_array, self->pulse_array_, sizeof(self->pulse_array_));
        std::sort(temp_array, temp_array + 11);

        self->ma3_pulse_width_ = temp_array[5] < 4166 ? temp_array[5] : self->ma3_pulse_width_;
    }
    return true;
}

uint32_t MotorDriver::getEncoderPosition() const //us
{
    return ma3_pulse_width_; // in microseconds
}

float MotorDriver::getPosition()
{
    position_ = (this->ma3_pulse_width_ - this->open_position_) / (this->closed_position_ - this->open_position_);
    return position_; // in [0,1]
}