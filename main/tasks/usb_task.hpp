#ifndef USB_TASK_HPP
#define USB_TASK_HPP

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
}
#include "usb_driver.hpp"

class UsbTask
{
public:
    UsbTask()
    {
        ESP_LOGI("UsbTask", "USB Task created");
        _usb = new UsbHidDevice();
        g_usb_hid_device_instance = _usb;
        _usb->init();
    }
    void start()
    {
        ESP_LOGI("UsbTask", "Starting USB Task");
        BaseType_t result = xTaskCreate(
            [](void *arg)
            {
                // Cast the argument back to UsbHidDevice* and call its taskLoop() method
                auto usb_ = static_cast<UsbHidDevice *>(arg);
                while(1)
                {
                    usb_->taskLoop();
                    vTaskDelay(pdMS_TO_TICKS(USB_RATE_MS)); // Adjust delay as needed
                }
            },
            "usb_hid_task", // Name of the task
            8192,           // Stack size (in bytes, adjust if needed based on usage)
            _usb,      // Parameter to pass to the task (our UsbHidDevice instance)
            5,              // Priority of the task (adjust as needed, higher is more urgent)
            &taskHandle     // Pass the address of taskHandle to receive the created task's handle
        );
        if (result != pdPASS) {
            ESP_LOGE("UsbTask", "Failed to create USB task");
            taskHandle = nullptr;
        }
    }
    void stop()
    {
        if (taskHandle != nullptr)
        {
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
        }
    }

    void startRTCUpdateTask()
    {
        ESP_LOGI("UsbTask", "Starting RTC Update Task");
        BaseType_t result = xTaskCreate(
            [](void *arg)
            {
                auto usb_ = static_cast<UsbHidDevice *>(arg);
                while (1)
                {
                    usb_->updateRTC();
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Update RTC every second
                }
            },
            "rtc_update_task", // Name of the task
            2048,              // Stack size (in bytes, adjust if needed based on usage)
            _usb,              // Parameter to pass to the task (our UsbHidDevice instance)
            5,                 // Priority of the task (adjust as needed, higher is more urgent)
            nullptr            // No need to capture the task handle for this one
        );
        if (result != pdPASS) {
            ESP_LOGE("UsbTask", "Failed to create RTC update task");
        }   
    }

private:
    static void taskFunction(void *arg);
    TaskHandle_t taskHandle;
    UsbHidDevice *_usb;
};

#endif // USB_TASK_HPP