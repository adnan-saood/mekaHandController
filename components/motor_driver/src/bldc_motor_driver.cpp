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

static const char *BLDC_TAG = "MotorDriverBLDC";


esp_err_t MotorDriverBLDC::setSpeed(float speed)
{
    if (!initialized_)
    {
        ESP_LOGE(BLDC_TAG, "MotorDriver not initialized");
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
