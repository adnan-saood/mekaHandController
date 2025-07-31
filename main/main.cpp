
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

#include "motor_driver.hpp"

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

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    MotorDriver motor(GPIO_NUM_5, 0, GPIO_NUM_4, 1);
    motor.init();
    motor.setSpeed(0.5f); // Set a test PWM value, adjust as needed
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to prevent busy-waiting
    }
}