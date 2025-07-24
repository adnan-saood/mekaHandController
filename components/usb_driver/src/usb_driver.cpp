#include "usb_driver.hpp"

extern "C" {
#include "driver/gpio.h"
#include "class/hid/hid_device.h"
}

#define APP_BUTTON GPIO_NUM_0
#define DISTANCE_MAX 125
#define DELTA 5

static const uint8_t hid_report_desc[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

static const uint8_t hid_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_report_desc), 0x81, 16, 10),
};

static const char* hid_str_desc[] = {
    (const char[]){0x09, 0x04},
    "TinyUSB",
    "ESP HID Mouse",
    "123456",
    "HID Interface",
};

extern "C" const uint8_t* tud_hid_descriptor_report_cb(uint8_t) {
    return hid_report_desc;
}

extern "C" uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t) {
    return 0;
}

extern "C" void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, const uint8_t*, uint16_t) {
    // Not used
}

UsbHidDevice::UsbHidDevice() : dir_(RIGHT), distance_(0) {
    mutex_ = xSemaphoreCreateMutex();
}

void UsbHidDevice::init() {
    const gpio_config_t btn_cfg = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn_cfg);

    const tinyusb_config_t tusb_cfg = {
        .string_descriptor = hid_str_desc,
        .string_descriptor_count = sizeof(hid_str_desc) / sizeof(hid_str_desc[0]),
        .external_phy = false,
#if TUD_OPT_HIGH_SPEED
        .fs_configuration_descriptor = hid_cfg_desc,
        .hs_configuration_descriptor = hid_cfg_desc,
#else
        .configuration_descriptor = hid_cfg_desc,
#endif
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}

void UsbHidDevice::nextMouseDelta(int8_t& dx, int8_t& dy) {
    dx = dy = 0;
    switch (dir_) {
        case RIGHT: dx = DELTA; break;
        case DOWN:  dy = DELTA; break;
        case LEFT:  dx = -DELTA; break;
        case UP:    dy = -DELTA; break;
    }
    distance_ += DELTA;
    if (distance_ >= DISTANCE_MAX) {
        distance_ = 0;
        dir_ = static_cast<MouseDir>((dir_ + 1) % 4);
    }
}

void UsbHidDevice::sendMouseSquareDemo() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10))) {
        int8_t dx, dy;
        for (int i = 0; i < 4 * (DISTANCE_MAX / DELTA); i++) {
            nextMouseDelta(dx, dy);
            tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, 0, dx, dy, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        xSemaphoreGive(mutex_);
    }
}

void UsbHidDevice::taskLoop() {
    while (1) {
        if (tud_mounted() && gpio_get_level(APP_BUTTON) == 0) {
            sendMouseSquareDemo();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
