#ifndef HEARTBEAT_TASK_HPP
#define HEARTBEAT_TASK_HPP

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
}

void HeartBeat()
{
    // configure LED 2 to blink in task as a heartbeat
    const gpio_config_t led_cfg = {
        .pin_bit_mask = BIT64(LED_D2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_cfg);
    // Main loop to blink the LED as a heartbeat
    // taskcreate
    xTaskCreate([](void *arg)
                {
        const gpio_num_t ledPin = LED_D2;
        while (1)
        {
            gpio_set_level(ledPin, 1);
            vTaskDelay(pdMS_TO_TICKS(50)); // LED ON for 50 ms
            gpio_set_level(ledPin, 0);
            vTaskDelay(pdMS_TO_TICKS(100)); // LED OFF for 100 ms
            gpio_set_level(ledPin, 1);
            vTaskDelay(pdMS_TO_TICKS(50)); // LED ON for 50 ms
            gpio_set_level(ledPin, 0);
            vTaskDelay(pdMS_TO_TICKS(600)); // LED OFF for 600 ms
        } }, "led_blink_task", 2048, NULL, 5, NULL);
}


#endif // !HEARTBEAT_TASK_HPP