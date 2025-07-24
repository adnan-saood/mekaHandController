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

class UsbHidDevice {
public:
    UsbHidDevice();
    void init(); // Initializes GPIO and TinyUSB
    void taskLoop(); // Main loop for sending and receiving

    // Method to handle incoming SET_REPORT (Output Report from PC)
    // This will be called by the tud_hid_set_report_cb extern "C" function.
    void handleSetReport(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize);

    // Method to send an incremented value back to the PC (Input Report)
    void sendIncrementedValue();

    // Public getters for mutex and data, specifically for extern "C" callbacks
    // These allow the C callbacks to safely access private members.
    SemaphoreHandle_t getMutex() { return mutex_; }
    uint8_t getValueToSendBack() { return value_to_send_back_; }

private:
    SemaphoreHandle_t mutex_; // Mutex for thread-safe access to class members
    uint8_t received_value_;      // Stores the last value received from PC
    uint8_t value_to_send_back_;  // Stores the value to send back to PC
    bool    new_value_available_; // Flag to indicate a new value needs to be sent back
};