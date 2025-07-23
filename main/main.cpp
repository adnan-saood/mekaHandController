
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h" // Add this include for ESP_LOGI

#include "tasks/logger_task.hpp"
#include "tasks/motor_task.hpp"
#include "tasks/sensor_task.hpp"

#include "usb_driver.hpp"

extern "C" void app_main(void) {
    ESP_LOGI("MAIN", "Starting application...");

    UsbHidDevice hid_device;

    if (!hid_device.init()) {
        ESP_LOGE("MAIN", "Failed to initialize USB HID device!");
        return;
    }

    // Example: Sending a simple report every second
    std::vector<uint8_t> tx_report(USB_HID_REPORT_SIZE, 0);
    uint8_t counter = 0;

    while (true) {
        if (hid_device.isConnected()) {
            tx_report[0] = 0xAA; // A magic byte
            tx_report[1] = counter++; // Incrementing counter
            tx_report[2] = 0x55; // Another magic byte

            if (hid_device.sendReport(tx_report)) {
                ESP_LOGI("MAIN", "Sent HID report: %02X %02X %02X...", tx_report[0], tx_report[1], tx_report[2]);
            } else {
                ESP_LOGW("MAIN", "Failed to send HID report (might not be connected yet).");
            }

            // Example: Try to receive a report if available
            std::vector<uint8_t> rx_report;
            if (hid_device.receiveReport(rx_report)) {
                ESP_LOGI("MAIN", "Received HID report of size %zu:", rx_report.size());
                // Print received data (for debugging)
                std::string received_str = "  ";
                for (uint8_t byte : rx_report) {
                    char buf[4];
                    sprintf(buf, "%02X ", byte);
                    received_str += buf;
                }
                ESP_LOGI("MAIN", "%s", received_str.c_str());
            }
        } else {
            ESP_LOGI("MAIN", "USB HID not connected, waiting...");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
    }
}