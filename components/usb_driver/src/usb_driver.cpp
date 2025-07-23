#include "usb_driver.hpp"

// ------------- TinyUSB C Callbacks -------------
extern "C" void tud_mount_cb(void) {
    if (UsbHidDevice::instance) UsbHidDevice::instance->onMount();
}

extern "C" void tud_umount_cb(void) {
    if (UsbHidDevice::instance) UsbHidDevice::instance->onUnmount();
}

