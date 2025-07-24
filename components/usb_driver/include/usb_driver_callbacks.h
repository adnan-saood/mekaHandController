#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "tusb.h" // Add this line to include TinyUSB types

#ifdef __cplusplus
extern "C" {
#endif

void usb_driver_init(void);
bool usb_device_mounted(void);
void usb_driver_mouse_square(void);

// TinyUSB callbacks
uint8_t const* tud_hid_descriptor_report_cb(uint8_t);
uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t);
void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const*, uint16_t);

#ifdef __cplusplus
}
#endif
