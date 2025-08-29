
extern "C"
{
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/gptimer.h"
#include "freertos/queue.h"
}

#include "config.h"

// #include "usb_driver.hpp"
#include <array>

#include "motor_driver.hpp"
// #include "sensor_task.hpp"
#include "usb_task.hpp"
// #include "adc.hpp"
#include "pin_config.h"

#include "heartbeat_task.hpp"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (8) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT // Set duty resolution to 10 bits
#define LEDC_DUTY (100)                 // Set duty to 10%. (2 ** 10) * 10% = 100
#define LEDC_FREQUENCY (240)            // Frequency in Hertz. Set frequency at 240 Hz

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num = LEDC_OUTPUT_IO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
////////////////////////////////////////////////////////////**

static bool encoder_callback(mcpwm_cap_channel_handle_t cap_chan,
                             const mcpwm_capture_event_data_t *edata,
                             void *user_data)
{
    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    static uint32_t cap_val_rise = 0;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS)
    {
        cap_val_rise = edata->cap_value;
    }
    else if (edata->cap_edge == MCPWM_CAP_EDGE_NEG)
    {
        uint32_t pulse_width = edata->cap_value - cap_val_rise;
        BaseType_t high_task_wakeup;
        xTaskNotifyFromISR(task_to_notify, pulse_width, eSetValueWithOverwrite, &high_task_wakeup);
        return high_task_wakeup == pdTRUE;
    }
    return false;
}

extern "C" void app_main(void)
{
    // Set the LEDC peripheral configuration
    example_ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    ESP_LOGI(TAG, "Install capture timer");
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .resolution_hz = 80000000 // 80 MHz
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = GPIO_NUM_14,
        .intr_priority = 1,
        .prescale = 1,
        // flags will be set below
    };
    cap_ch_conf.flags.pos_edge = true;
    cap_ch_conf.flags.neg_edge = true;
    cap_ch_conf.flags.pull_up = false;
    cap_ch_conf.flags.pull_down = false;
    cap_ch_conf.flags.invert_cap_signal = false;

    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));

    ESP_LOGI(TAG, "Register capture callback");
    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = encoder_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, cur_task));

    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));

    uint32_t tof_ticks;
    while (1)
    {
        if (xTaskNotifyWait(0, ULONG_MAX, &tof_ticks, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            float pulse_width_us = (float)tof_ticks / 80.0; // since 1 tick = 1 µs
            ESP_LOGI(TAG, "Measured pulse: %.1f us", pulse_width_us);
        }
    }
}
