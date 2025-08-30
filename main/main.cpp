
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

MotorDriver m(0,GPIO_NUM_15, GPIO_NUM_10, GPIO_NUM_14);

extern "C" void app_main(void)
{
    // Set the LEDC peripheral configuration
    example_ledc_init();
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    m.init();

    while(1)
    {
        uint32_t position = m.getEncoderPosition();
        ESP_LOGI(TAG, "Encoder position: %lu", position);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
