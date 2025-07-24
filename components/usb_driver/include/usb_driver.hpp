#pragma once

extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "tinyusb.h"
}

class UsbHidDevice {
public:
    UsbHidDevice();
    void init();
    void sendMouseSquareDemo();
    void taskLoop();

private:
    void nextMouseDelta(int8_t& dx, int8_t& dy);

    enum MouseDir { RIGHT, DOWN, LEFT, UP };
    MouseDir dir_;
    uint32_t distance_;
    SemaphoreHandle_t mutex_;
};
