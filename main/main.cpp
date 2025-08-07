
extern "C"
{
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"
    // #include "usb_driver_callbacks.h"
}

#include "usb_driver.hpp"
#include <array>

#include "motor_driver.hpp"
#include "pin_config.h"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

extern "C" void app_main(void)
{
    // static UsbHidDevice usb;
    // usb.init();
    // xTaskCreate([](void*) { usb.taskLoop(); }, "usb_loop", 4096, nullptr, 5, nullptr);

    // Init CDC through UART0 and print a message
    ESP_LOGI(TAG, "Starting USB HID device...");
    printf("USB HID device starting...\n");

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
        [](void *arg)
        {
            // Cast the argument back to UsbHidDevice* and call its taskLoop() method
            static_cast<UsbHidDevice *>(arg)->taskLoop();
        },
        "usb_hid_task", // Name of the task
        8192,           // Stack size (in bytes, adjust if needed based on usage)
        &myHidDevice,   // Parameter to pass to the task (our UsbHidDevice instance)
        5,              // Priority of the task (adjust as needed, higher is more urgent)
        NULL            // Task handle (we don't need to store it for this example)
    );

    // Create MotorDriver for MCPWM unit 0, high side GPIO 5, low side GPIO 18 (example)
    MotorDriver motor(0, MOTOR_J2_PWM_H, MOTOR_J2_PWM_L);

    if (motor.init() == ESP_OK)
    {
        // Set speed to 50% forward as a test
        motor.setSpeed(0);
    }

    while (1)
    {
        // print current usb poses
        std::array<float, 5> current_positions;
        for (size_t i = 0; i < 5; ++i)
        {
            current_positions[i] = (g_usb_hid_device_instance->getCommandedPoses(i) - 127.5f) / 255.0f; // Adjusting to a range of [-0.5, 0.5]
            printf("Current USB poses: ");
            for (size_t i = 0; i < current_positions.size(); ++i)
            {
                printf("%.3f%s", current_positions[i], (i < current_positions.size() - 1) ? ", " : "\n");
            }
        }
        while (current_positions[0] == -0.5f)
        {
            current_positions[0] = (g_usb_hid_device_instance->getCommandedPoses(0) - 127.5f) / 255.0f; // Adjusting to a range of [-0.5, 0.5]
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Set speed to 50% forward as a test
        motor.setSpeed(current_positions[0]);
        // Delay to prevent busy-waiting
        vTaskDelay(pdMS_TO_TICKS(50)); // Delay to prevent busy-waiting
    }
}