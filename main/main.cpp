
extern "C"
{
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/uart.h"
}

#include "config.h"

// #include "usb_driver.hpp"
#include <array>

#include "motor_driver.hpp"
// #include "sensor_task.hpp"
#include "usb_task.hpp"
// #include "adc.hpp"
#include "pin_config.h"

#include "heartbeat_task.hpp"

#define APP_BUTTON GPIO_NUM_0
static const char *TAG = "main";

MotorDriver m0(0, MOTOR_J0_PWM, MOTOR_J0_DIR);
MotorDriver m1(1, MOTOR_J1_PWM_H, MOTOR_J1_PWM_L);
MotorDriver m2(0, MOTOR_J2_PWM_H, MOTOR_J2_PWM_L);
MotorDriver m3(0, MOTOR_J3_PWM_H, MOTOR_J3_PWM_L);
MotorDriver m4(1, MOTOR_J4_PWM_H, MOTOR_J4_PWM_L);

bool hand_open = true;

void close_hand()
{
    float speed = 0.6;
    int delay = 2000;
    
    m1.setSpeed(-speed);
    m2.setSpeed(speed);
    m3.setSpeed(-speed);
    m4.setSpeed(-speed);

    vTaskDelay(pdMS_TO_TICKS(delay)); // Delay to prevent busy-waiting
    m1.setSpeed(0.0);
    m2.setSpeed(0.0);
    m3.setSpeed(0.0);
    m4.setSpeed(0.0);

    gpio_set_level(LED_D3, 1);
    hand_open = false;
}

void open_hand()
{
    float speed = 0.6;
    int delay = 2000;

    m1.setSpeed(speed);
    m2.setSpeed(-speed);
    m3.setSpeed(speed);
    m4.setSpeed(speed);

    vTaskDelay(pdMS_TO_TICKS(delay)); // Delay to prevent busy-waiting
    m1.setSpeed(0.0);
    m2.setSpeed(0.0);
    m3.setSpeed(0.0);
    m4.setSpeed(0.0);
    // light down D3
    gpio_set_level(LED_D3, 0);
    hand_open = true;
}

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(5000));
    UsbTask usbTask;
    usbTask.start();
    HeartBeat();

    // initialize d3 as led
    const gpio_config_t led_cfg = {
        .pin_bit_mask = BIT64(LED_D3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_cfg);

    // pass usb object
    xTaskCreate([](void *arg)
                {
        auto usb = static_cast<UsbHidDevice *>(arg);
        while (1) {
            if(usb->getCommandedPoses(0) == 0)
            {
                if(hand_open)
                    close_hand();
            }
            if(usb->getCommandedPoses(0) == 1)
            {
                if(!hand_open)
                    open_hand();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        } }, "listen_to_usb", 4096, usbTask._usb, 5, NULL);

    m0.init();
    m1.init();
    m2.init();
    m3.init();
    m4.init();

    // stop all motors
    m0.setSpeed(0);
    m1.setSpeed(0);
    m2.setSpeed(0);
    m3.setSpeed(0);
    m4.setSpeed(0);

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to prevent busy-waiting
    }
}
