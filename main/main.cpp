
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
#include "driver/gptimer.h"
#include "freertos/queue.h"
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

MotorDriver m0("motor_j0_", 0, MOTOR_J0_PWM, MOTOR_J0_DIR, MOTOR_ENC_0);
MotorDriver m1("motor_j1_", 0, MOTOR_J1_PWM_H, MOTOR_J1_PWM_L, MOTOR_ENC_1);
MotorDriver m2("motor_j2_", 0, MOTOR_J2_PWM_H, MOTOR_J2_PWM_L, MOTOR_ENC_2);
MotorDriver m3("motor_j3_", 1, MOTOR_J3_PWM_H, MOTOR_J3_PWM_L, MOTOR_ENC_3);
MotorDriver m4("motor_j4_", 1, MOTOR_J4_PWM_H, MOTOR_J4_PWM_L, MOTOR_ENC_4);

extern "C" void app_main(void)
{
    HeartBeat();


    vTaskDelay(pdMS_TO_TICKS(4000)); // wait for 1 second
    m0.init();
    m1.init();
    m2.init();
    m3.init();
    m4.init();

    UsbTask usb_task;
    usb_task.start();

    m1.startPositionControl();
    m3.startPositionControl();
    m4.startPositionControl();

    // xTaskCreate([](void *arg) {
    //     MotorDriver *motor = static_cast<MotorDriver *>(arg);
    //     while(1){
    //         motor->setSpeed(-0.3);
    //         vTaskDelay(pdMS_TO_TICKS(4000));
    //         motor->setSpeed(0);
    //         vTaskDelay(pdMS_TO_TICKS(3000));
    //         motor->setSpeed(0.3);
    //         vTaskDelay(pdMS_TO_TICKS(4000));
    //         motor->setSpeed(0);
    //     }
    // }, "PrintMotorPosition", 4096, &m4, 10, nullptr);

    while (1)
    {
        m1.setPosition(static_cast<float>(usb_task.getCommandedPose(1)) / 255.0f);
        m1.setGains(- static_cast<float>(usb_task.getCommandedStiffness(1)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(2)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(3)) / 51.0f);


        m3.setPosition(static_cast<float>(usb_task.getCommandedPose(3)) / 255.0f);
        m3.setGains(- static_cast<float>(usb_task.getCommandedStiffness(1)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(2)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(3)) / 51.0f);

        m4.setPosition(static_cast<float>(usb_task.getCommandedPose(4)) / 255.0f);
        m4.setGains(- static_cast<float>(usb_task.getCommandedStiffness(1)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(2)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(3)) / 51.0f);

        

        float m1_velocity = m1.getSpeed();
        usb_task._usb->getPayloadPointer()[20] = static_cast<uint8_t>(m1_velocity * 125 + 125); // velocity in centi-units
        float m3_velocity = m3.getSpeed();
        usb_task._usb->getPayloadPointer()[21] = static_cast<uint8_t>(m3_velocity * 125 + 125); // velocity in centi-units
        float m4_velocity = m4.getSpeed();
        usb_task._usb->getPayloadPointer()[23] = static_cast<uint8_t>(m4_velocity * 125 + 125); // velocity in centi-units

        float m1_position = m1.getPosition();
        usb_task._usb->getPayloadPointer()[1] = static_cast<uint8_t>(m1_position * 255); // position in units
        float m3_position = m3.getPosition();
        usb_task._usb->getPayloadPointer()[3] = static_cast<uint8_t>(m3_position * 255); // position in units
        float m4_position = m4.getPosition();
        usb_task._usb->getPayloadPointer()[4] = static_cast<uint8_t>(m4_position * 255); // position in units

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
