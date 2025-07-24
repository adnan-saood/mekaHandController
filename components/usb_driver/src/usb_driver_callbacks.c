#include "usb_driver_callbacks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

#define TAG "usb_driver"

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define DISTANCE_MAX 125
#define DELTA_SCALAR 5

typedef enum {
    MOUSE_DIR_RIGHT,
    MOUSE_DIR_DOWN,
    MOUSE_DIR_LEFT,
    MOUSE_DIR_UP,
    MOUSE_DIR_MAX
} mouse_dir_t;

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(1))
};

static const char *hid_string_descriptor[] = {
    (const char[]){ 0x09, 0x04 }, // English
    "Espressif",
    "ESP HID Mouse",
    "123456",
    "HID Interface"
};

static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, 0, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 8, 10)
};

void usb_driver_init(void)
{
    const tinyusb_config_t cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor,
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));
}

bool usb_device_mounted(void)
{
    return tud_mounted();
}

void usb_driver_mouse_square(void)
{
    static mouse_dir_t dir = MOUSE_DIR_RIGHT;
    static uint32_t dist = 0;

    int8_t dx = 0, dy = 0;
    switch (dir) {
        case MOUSE_DIR_RIGHT: dx = DELTA_SCALAR; break;
        case MOUSE_DIR_DOWN:  dy = DELTA_SCALAR; break;
        case MOUSE_DIR_LEFT:  dx = -DELTA_SCALAR; break;
        case MOUSE_DIR_UP:    dy = -DELTA_SCALAR; break;
        default: break;
    }

    tud_hid_mouse_report(1, 0x00, dx, dy, 0, 0);

    dist += DELTA_SCALAR;
    if (dist >= DISTANCE_MAX) {
        dist = 0;
        dir = (mouse_dir_t)((dir + 1) % MOUSE_DIR_MAX);
    }
}

/*********** TinyUSB Callbacks ***********/

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}
