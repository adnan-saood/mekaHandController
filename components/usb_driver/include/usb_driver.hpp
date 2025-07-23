#pragma once

#include <cstdint>
#include <cstring>
#include <array>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "class/hid/hid_device.h"
#include "tusb.h"
}

class UsbHidDevice {
public:
    static constexpr size_t REPORT_SIZE = 64;

    UsbHidDevice() {
        tx_sem = xSemaphoreCreateBinary();
        rx_sem = xSemaphoreCreateBinary();
        connected = false;
        instance = this;
    }

    bool init() {
        // Nothing to init here since TinyUSB is configured via menuconfig
        ESP_LOGI(TAG, "UsbHidDevice initialized");
        return true;
    }

    bool sendReport(const uint8_t* data, size_t len) {
        if (!connected || len > REPORT_SIZE) return false;

        if (tud_hid_ready()) {
            tud_hid_report(0, data, len);
            return true;
        }

        return false;
    }

    bool receiveReport(uint8_t* out_buf, size_t* out_len) {
        if (xSemaphoreTake(rx_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
            memcpy(out_buf, last_rx_report.data(), REPORT_SIZE);
            *out_len = REPORT_SIZE;
            return true;
        }
        return false;
    }

    bool isConnected() const {
        return connected;
    }

    // These will be called from the extern "C" section
    void onMount() { connected = true; }
    void onUnmount() { connected = false; }
    void onSetReport(uint8_t, uint8_t, uint8_t, const uint8_t* buf, uint16_t len) {
        memcpy(last_rx_report.data(), buf, len);
        xSemaphoreGive(rx_sem);
    }

private:
    static constexpr const char* TAG = "UsbHid";
    bool connected;
    SemaphoreHandle_t tx_sem;
    SemaphoreHandle_t rx_sem;
    std::array<uint8_t, REPORT_SIZE> last_rx_report;

    static inline UsbHidDevice* instance = nullptr;

    // Allow TinyUSB C callbacks to access the instance
    friend void tud_mount_cb(void);
    friend void tud_umount_cb(void);
    friend void tud_hid_set_report_cb(uint8_t inst, uint8_t id, uint8_t type,
                                      const uint8_t* buf, uint16_t len){
    if (UsbHidDevice::instance) {
        UsbHidDevice::instance->onSetReport(inst, id, type, buf, len);
    }
}
};
