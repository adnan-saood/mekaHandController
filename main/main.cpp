
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h" // Add this include for ESP_LOGI

#include "tasks/logger_task.hpp"
#include "tasks/motor_task.hpp"
#include "tasks/sensor_task.hpp"

#include "usb_driver.hpp"

UsbHidDevice usb;

extern "C" void app_main(void) {
   usb.init();

    uint8_t dummy_report[64] = {0x01, 0x02, 0x03};
    while (true) {
        if (usb.isConnected()) {
            usb.sendReport(dummy_report, 64);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}