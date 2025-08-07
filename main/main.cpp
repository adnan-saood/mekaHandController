
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
#include "sensor_task.hpp"
#include "adc.hpp"
#include "pin_config.h"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

void HearBeat();

extern "C" void app_main(void)
{
    // static UsbHidDevice usb;
    // usb.init();
    // xTaskCreate([](void*) { usb.taskLoop(); }, "usb_loop", 4096, nullptr, 5, nullptr);

    // Init CDC through UART0 and print a message
    ESP_LOGI(TAG, "Starting USB HID device...");
    printf("USB HID device starting...\n");

    // // 1. Create an instance of your UsbHidDevice class
    // UsbHidDevice myHidDevice;

    // // 2. Assign the address of your instance to the global pointer
    // // This is crucial for the extern "C" TinyUSB callbacks to function correctly.
    // g_usb_hid_device_instance = &myHidDevice;

    // // 3. Initialize the USB HID device
    // // This sets up GPIO and installs the TinyUSB driver.
    // myHidDevice.init();

    // // 4. Create a FreeRTOS task to run the device's main loop
    // // The taskLoop() method contains the infinite loop for handling USB events and button presses.
    // xTaskCreate(
    //     [](void *arg)
    //     {
    //         // Cast the argument back to UsbHidDevice* and call its taskLoop() method
    //         static_cast<UsbHidDevice *>(arg)->taskLoop();
    //     },
    //     "usb_hid_task", // Name of the task
    //     8192,           // Stack size (in bytes, adjust if needed based on usage)
    //     &myHidDevice,   // Parameter to pass to the task (our UsbHidDevice instance)
    //     5,              // Priority of the task (adjust as needed, higher is more urgent)
    //     NULL            // Task handle (we don't need to store it for this example)
    // );


    HearBeat();

    // IMU imu;

    // imu.init();

    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for IMU initialization
    


// [5 Poses and 5 Velocities and 5 Forces and 4 quaternion values and 5 MA3 Encoder values and 13 ADC values]
// [0 : 4] Poses
    // [5 : 9] Velocities
    // [10 : 14] Forces
    // [15 : 18] Quaternion values
    // [19 : 23] MA3 Encoder values
    // [24 : 36] ADC values
    while (1)
    {
        // IMUData imuData = imu.read();

        int8_t payload[4] = {0};
         for (size_t i = 0; i < 4; ++i) {
            // payload[i] = static_cast<int8_t>(imuData.quaternion[i] * 127.0f); // Scale to int8_t range
        }

        // Process IMU data (e.g., send to USB)
        // xSemaphoreTake(myHidDevice.getMutex(), pdMS_TO_TICKS(10));
        // memcpy(myHidDevice.getPayloadPointer() + 15, payload, 4);
        // xSemaphoreGive(myHidDevice.getMutex());

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay to prevent busy-waiting
    }


}

void HearBeat()
{
    // configure LED 2 to blink in task as a heartbeat
    const gpio_config_t led_cfg = {
        .pin_bit_mask = BIT64(LED_D2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_cfg);
    // Main loop to blink the LED as a heartbeat
    // taskcreate
    xTaskCreate([](void *arg)
                {
        const gpio_num_t ledPin = LED_D2;
        while (1)
        {
            gpio_set_level(ledPin, 1);
            vTaskDelay(pdMS_TO_TICKS(50)); // LED ON for 500 ms
            gpio_set_level(ledPin, 0);
            vTaskDelay(pdMS_TO_TICKS(100)); // LED OFF for 500 ms
            gpio_set_level(ledPin, 1);
            vTaskDelay(pdMS_TO_TICKS(50)); // LED ON for 500 ms
            gpio_set_level(ledPin, 0);
            vTaskDelay(pdMS_TO_TICKS(600)); // LED OFF for 500 ms
        } }, "led_blink_task", 2048, NULL, 5, NULL);
}
