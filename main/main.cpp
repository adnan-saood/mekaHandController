
extern "C" {
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
    // #include "usb_driver_callbacks.h"
}

#include "usb_driver.hpp"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

extern "C" void app_main(void) {
    // static UsbHidDevice usb;
    // usb.init();
    // xTaskCreate([](void*) { usb.taskLoop(); }, "usb_loop", 4096, nullptr, 5, nullptr);


        // 1. Create an instance of your UsbHidDevice class
    UsbHidDevice myHidDevice;

    // 2. Assign the address of your instance to the global pointer
    // This is crucial for the extern "C" TinyUSB callbacks to function correctly.
    g_usb_hid_device_instance = &myHidDevice;

    // 3. Initialize the USB HID device
    // This sets up GPIO and installs the TinyUSB driver.
    myHidDevice.init();

    // 4. Create a FreeRTOS task to run the device's main loop
    // The taskLoop() method contains the infinite loop for handling USB events and button presses.
    xTaskCreate(
        [](void* arg) {
            // Cast the argument back to UsbHidDevice* and call its taskLoop() method
            static_cast<UsbHidDevice*>(arg)->taskLoop();
        },
        "usb_hid_task",   // Name of the task
        8192,             // Stack size (in bytes, adjust if needed based on usage)
        &myHidDevice,     // Parameter to pass to the task (our UsbHidDevice instance)
        5,                // Priority of the task (adjust as needed, higher is more urgent)
        NULL              // Task handle (we don't need to store it for this example)
    );
}