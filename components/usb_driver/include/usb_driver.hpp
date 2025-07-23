#pragma once

#include <cstdint>
#include <vector>
#include <string> // For logging

// Define report structure for USB HID
#define USB_HID_REPORT_SIZE 64
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h" // For ESP_OK, ESP_FAIL
#include "tinyusb.h"
#include "tinyusb/class/hid/hid.h" // Example: Could be tinyusb/class/hid/hid.h for generic HID

}

#include "sensors.hpp" // Include sensor data structures
#include "config.h" // Include configuration settings

// Tag for ESP_LOG
static const char *TAG = "UsbHidDevice";

// Forward declaration for TinyUSB callbacks
extern "C" {
void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance);
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
}

// Static instance to allow callbacks to access class members (a common pattern with C callbacks)
static UsbHidDevice* s_usb_hid_device_instance = nullptr;

// HID Report Descriptor for 5 motors (position & stiffness control), and sensor feedback
// - Output report: Set position (5x16-bit), stiffness (5x8-bit)
// - Input report: Position (5x16-bit), Velocity (5x16-bit), Force (5x16-bit), IMU (6x16-bit: accel/gyro XYZ), ADC (8x16-bit)
// Report IDs: 1 (Input), 2 (Output)

static const uint8_t _hid_report_descriptor[] = {
    // Output Report (Report ID 2): Motor control
    0x06, 0x00, 0xFF,      // Usage Page (Vendor Defined)
    0x09, 0x01,            // Usage (Vendor Usage 1)
    0xA1, 0x01,            // Collection (Application)
    0x85, 0x02,            //   Report ID (2)
    // Motor Position (5 x 16-bit)
    0x75, 0x10,            //   Report Size (16)
    0x95, 0x05,            //   Report Count (5)
    0x09, 0x10,            //   Usage (Motor Position)
    0x91, 0x02,            //   Output (Data,Var,Abs)
    // Motor Stiffness (5 x 8-bit)
    0x75, 0x08,            //   Report Size (8)
    0x95, 0x05,            //   Report Count (5)
    0x09, 0x11,            //   Usage (Motor Stiffness)
    0x91, 0x02,            //   Output (Data,Var,Abs)
    0xC0,                  // End Collection

    // Input Report (Report ID 1): Sensor feedback
    0x06, 0x00, 0xFF,      // Usage Page (Vendor Defined)
    0x09, 0x02,            // Usage (Vendor Usage 2)
    0xA1, 0x01,            // Collection (Application)
    0x85, 0x01,            //   Report ID (1)
    // Motor Position (5 x 16-bit)
    0x75, 0x10,            //   Report Size (16)
    0x95, 0x05,            //   Report Count (5)
    0x09, 0x20,            //   Usage (Motor Position Feedback)
    0x81, 0x02,            //   Input (Data,Var,Abs)
    // Motor Velocity (5 x 16-bit)
    0x75, 0x10,
    0x95, 0x05,
    0x09, 0x21,            //   Usage (Motor Velocity)
    0x81, 0x02,
    // Motor Force (5 x 16-bit)
    0x75, 0x10,
    0x95, 0x05,
    0x09, 0x22,            //   Usage (Motor Force)
    0x81, 0x02,
    // IMU Data (Accel XYZ, Gyro XYZ: 6 x 16-bit)
    0x75, 0x10,
    0x95, 0x06,
    0x09, 0x30,            //   Usage (IMU Data)
    0x81, 0x02,
    // ADC Data (8 x 16-bit)
    0x75, 0x10,
    0x95, 0x08,
    0x09, 0x40,            //   Usage (ADC Data)
    0x81, 0x02,
    0xC0                   // End Collection
};


UsbHidDevice::UsbHidDevice() : connected(false) {
    s_usb_hid_device_instance = this; // Set the static instance
    tx_ready_sem = xSemaphoreCreateBinary();
    rx_data_sem = xSemaphoreCreateBinary();
    received_report_buffer.resize(USB_HID_REPORT_SIZE); // Pre-allocate buffer
    if (tx_ready_sem == NULL || rx_data_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphores");
        // Handle error appropriately, e.g., throw an exception or set an error flag
    }
}

UsbHidDevice::~UsbHidDevice() {
    if (tx_ready_sem) {
        vSemaphoreDelete(tx_ready_sem);
    }
    if (rx_data_sem) {
        vSemaphoreDelete(rx_data_sem);
    }
    s_usb_hid_device_instance = nullptr;
}

bool UsbHidDevice::init() {
    ESP_LOGI(TAG, "Initializing USB HID Device...");

    tusb_desc_device_t hid_descriptor = {
        .bLength = sizeof(hid_descriptor),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200, // USB 2.0
        .bDeviceClass = 0x00, // Implemented by the interface descriptors
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = USB_VID, // From config.h
        .idProduct = USB_PID, // From config.h
        .bcdDevice = 0x0100, // Device release number

        .iManufacturer = 0x01, // Index of string descriptor
        .iProduct = 0x02,      // Index of string descriptor
        .iSerialNumber = 0x03, // Index of string descriptor

        .bNumConfigurations = 0x01
    };

    // Configure TinyUSB with our device descriptor and callbacks
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = &hid_descriptor,
        .string_descriptor = (const char**) config_usb_string_descriptors, // From config.h
        .external_phy = false, // Use internal PHY for ESP32-S3
        .configuration_descriptor = NULL, // Not explicitly needed if using default TUD_HID_DESCRIPTOR
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    ESP_LOGI(TAG, "USB HID Device initialized.");
    return true;
}

bool UsbHidDevice::sendReport(const std::vector<uint8_t>& report) {
    if (!connected) {
        ESP_LOGW(TAG, "Device not connected, cannot send report.");
        return false;
    }

    if (report.size() > USB_HID_REPORT_SIZE) {
        ESP_LOGE(TAG, "Report size (%zu) exceeds max allowed (%d)", report.size(), USB_HID_REPORT_SIZE);
        return false;
    }

    // Block until the HID interface is ready to send (host has enumerated it)
    if (xSemaphoreTake(tx_ready_sem, pdMS_TO_TICKS(100)) != pdTRUE) { // Adjust timeout as needed
        ESP_LOGW(TAG, "Timed out waiting for HID TX ready semaphore.");
        return false;
    }

    // tud_hid_report takes report_id, report, len. For single report devices, report_id is 0.
    bool sent_ok = tud_hid_report(0, report.data(), report.size());
    if (!sent_ok) {
        ESP_LOGE(TAG, "Failed to send HID report.");
    } else {
        ESP_LOGD(TAG, "HID report sent successfully.");
    }
    return sent_ok;
}

bool UsbHidDevice::receiveReport(std::vector<uint8_t>& report) {
    if (!connected) {
        ESP_LOGW(TAG, "Device not connected, no report to receive.");
        return false;
    }

    // Wait for a new report to be received
    if (xSemaphoreTake(rx_data_sem, pdMS_TO_TICKS(portMAX_DELAY)) != pdTRUE) {
        ESP_LOGW(TAG, "Timed out waiting for HID RX data semaphore.");
        return false;
    }

    // Copy received data to the user's report vector
    report = received_report_buffer;
    ESP_LOGD(TAG, "HID report received, size: %zu", report.size());
    return true;
}

bool UsbHidDevice::isConnected() const {
    return connected;
}

// --- Internal callback handlers (called by friend C functions) ---
void UsbHidDevice::on_mount() {
    connected = true;
    ESP_LOGI(TAG, "USB HID mounted.");
    // Give semaphore to allow sending reports after enumeration
    xSemaphoreGive(tx_ready_sem);
}

void UsbHidDevice::on_unmount() {
    connected = false;
    ESP_LOGI(TAG, "USB HID unmounted.");
}

void UsbHidDevice::on_suspend(bool remote_wakeup_en) {
    ESP_LOGW(TAG, "USB HID suspended. Remote wakeup enabled: %d", remote_wakeup_en);
}

void UsbHidDevice::on_resume() {
    ESP_LOGI(TAG, "USB HID resumed.");
}

void UsbHidDevice::on_set_report(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    ESP_LOGI(TAG, "Received HID report (instance=%d, id=%d, type=%d, size=%d)", instance, report_id, report_type, bufsize);
    if (bufsize > received_report_buffer.size()) {
        ESP_LOGW(TAG, "Received report size (%u) exceeds buffer size (%zu), truncating.", bufsize, received_report_buffer.size());
        bufsize = received_report_buffer.size();
    }
    memcpy(received_report_buffer.data(), buffer, bufsize);
    received_report_buffer.resize(bufsize); // Adjust size to actual received data
    xSemaphoreGive(rx_data_sem); // Signal that new data is available
}


// --- TinyUSB C Callbacks (must be extern "C") ---

// Invoked when device is mounted (configured)
void tud_mount_cb(void) {
    if (s_usb_hid_device_instance) {
        s_usb_hid_device_instance->on_mount();
    }
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    if (s_usb_hid_device_instance) {
        s_usb_hid_device_instance->on_unmount();
    }
}

// Invoked when USB bus is suspended
// remote_wakeup_en: If host allows us to send remote wakeup
void tud_suspend_cb(bool remote_wakeup_en) {
    if (s_usb_hid_device_instance) {
        s_usb_hid_device_instance->on_suspend(remote_wakeup_en);
    }
}

// Invoked when USB bus is resumed
void tud_resume_cb(void) {
    if (s_usb_hid_device_instance) {
        s_usb_hid_device_instance->on_resume();
    }
}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    // We only have one HID instance (0)
    (void) instance;
    return UsbHidDevice::_hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer with data as specified by report_id (if any) and report_type
// Returns number of bytes copied to buffer
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    // This callback is usually for host requesting input reports (e.g., polling) or feature reports.
    // For simplicity, we'll return 0, meaning no data is provided by the device on host request.
    // In a real application, you might fill 'buffer' with current sensor data if the host polls.
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or received data via OUT endpoint (for output reports)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    if (s_usb_hid_device_instance) {
        s_usb_hid_device_instance->on_set_report(instance, report_id, report_type, buffer, bufsize);
    }
}