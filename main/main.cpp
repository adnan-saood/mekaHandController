
extern "C" {
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
    // #include "usb_driver_callbacks.h"
}

#include "usb_driver.hpp"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

extern "C" void app_main(void) {
    static UsbHidDevice usb;
    usb.init();
    xTaskCreate([](void*) { usb.taskLoop(); }, "usb_loop", 4096, nullptr, 5, nullptr);
}