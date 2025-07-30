// usb_driver.hpp
#pragma once

extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "tinyusb.h"
}

// Forward declaration of the C++ class to be used in C callbacks
class UsbHidDevice;

// Global pointer to your C++ UsbHidDevice instance.
// This allows the extern "C" TinyUSB callbacks to access your C++ object.
extern UsbHidDevice* g_usb_hid_device_instance;

/*
OUTPUT: [5 timestamp + 5 poses + 5 stiffness values]
*/

class UsbHidDevice {
public:
    UsbHidDevice();
    void init(); // Initializes GPIO and TinyUSB
    void taskLoop(); // Main loop for sending and receiving
    void othertaskLoop();

    // Method to handle incoming SET_REPORT (Output Report from PC)
    // This will be called by the tud_hid_set_report_cb extern "C" function.
    void handleSetReport(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize);

    // Method to send an incremented value back to the PC (Input Report)
    void sendData();

    // Public getters for mutex and data, specifically for extern "C" callbacks
    // These allow the C callbacks to safely access private members.
    SemaphoreHandle_t getMutex() { return mutex_; }
    uint8_t* getValueToSendBack() { return payload_data_; }

    uint8_t getCommandedPoses(uint8_t motor_index) { 
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10))) {
            memcpy(poses_, received_packet_ + 5, sizeof(poses_));
            xSemaphoreGive(mutex_);
        }
        return poses_[motor_index];
    }

    uint8_t getCommandedStiffness(uint8_t motor_index) {
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10))) {
            memcpy(stiffness_, received_packet_ + 10, sizeof(stiffness_));
            xSemaphoreGive(mutex_);
        }
        return stiffness_[motor_index];
    }

    void updateRTC();

    private:
    SemaphoreHandle_t mutex_; // Mutex for thread-safe access to class members
    uint8_t received_packet_[15] = {0};      // Stores the last value received from PC
    uint8_t payload_data_[37] = {0}; // Buffer to hold the payload data
    uint8_t poses_[5] = {0}; // Stores the commanded poses
    uint8_t stiffness_[5] = {0}; // Stores the commanded stiffness values
    SemaphoreHandle_t data_mutex_;
    bool    new_value_available_; // Flag to indicate a new value needs to be sent back
};