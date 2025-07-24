
extern "C" {
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
    #include "usb_driver_callbacks.h"
}

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

extern "C" void app_main(void)
{
    gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << APP_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    usb_driver_init();
    ESP_LOGI(TAG, "USB HID initialized");

    while (true) {
        if (usb_device_mounted() && gpio_get_level(APP_BUTTON) == 0) {
            usb_driver_mouse_square();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
