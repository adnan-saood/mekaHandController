
extern "C"
{
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"
    // #include "usb_driver_callbacks.h"
}

#include "usb_driver.hpp"
#include <array>

#include "motor_driver.hpp"
#include "sensor_task.hpp"
#include "usb_task.hpp"
#include "adc.hpp"
#include "pin_config.h"

#include "heartbeat_task.hpp"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

extern "C" void app_main(void)
{
    // UsbTask usbTask;
    // usbTask.start();
    HeartBeat();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay to prevent busy-waiting
    }
}

