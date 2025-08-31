
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

MotorDriver m1("motor_j1_", 0, MOTOR_J1_PWM_H, MOTOR_J1_PWM_L, MOTOR_ENC_1);
MotorDriver m2("motor_j2_", 0, MOTOR_J2_PWM_H, MOTOR_J2_PWM_L, MOTOR_ENC_2);
MotorDriver m3("motor_j3_", 1, MOTOR_J3_PWM_H, MOTOR_J3_PWM_L, MOTOR_ENC_3);
MotorDriver m4("motor_j4_", 1, MOTOR_J4_PWM_H, MOTOR_J4_PWM_L, MOTOR_ENC_4);

extern "C" void app_main(void)
{
    HeartBeat();


    vTaskDelay(pdMS_TO_TICKS(6000)); // wait for 6 seconds
    m1.init();
    m2.init();
    m3.init();
    m4.init();

    UsbTask usb_task;
    usb_task.start();

    m1.setEncoderLimits(1000.0f, 3000.0f);
    m2.setEncoderLimits(1000.0f, 3000.0f);
    m3.setEncoderLimits(1000.0f, 3000.0f);
    m4.setEncoderLimits(1000.0f, 3500.0f);

    m1.startPositionControl();
    // m2.startPositionControl();
    m3.startPositionControl();
    m4.startPositionControl();

    while (1)
    {
        m1.setPosition(static_cast<float>(usb_task.getCommandedPose(1)) / 255.0f);
        m1.setGains(- static_cast<float>(usb_task.getCommandedStiffness(1)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(2)) / 51.0f,
                    - static_cast<float>(usb_task.getCommandedStiffness(3)) / 51.0f);

        m2.setPosition(static_cast<float>(usb_task.getCommandedPose(2)) / 255.0f);
        m2.setGains(- static_cast<float>(usb_task.getCommandedStiffness(1)) / 51.0f,
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
        usb_task._usb->getPayloadPointer()[6] = static_cast<uint8_t>(m1_velocity * 127 + 127); // velocity in centi-units
        float m2_velocity = m2.getSpeed();
        usb_task._usb->getPayloadPointer()[7] = static_cast<uint8_t>(m2_velocity * 127 + 127); // velocity in centi-units
        float m3_velocity = m3.getSpeed();
        usb_task._usb->getPayloadPointer()[8] = static_cast<uint8_t>(m3_velocity * 127 + 127); // velocity in centi-units
        float m4_velocity = m4.getSpeed();
        usb_task._usb->getPayloadPointer()[9] = static_cast<uint8_t>(m4_velocity * 127 + 127); // velocity in centi-units

        float m1_position = m1.getPosition();
        usb_task._usb->getPayloadPointer()[1] = static_cast<uint8_t>(m1_position * 255); // position in units
        float m2_position = m2.getPosition();
        usb_task._usb->getPayloadPointer()[2] = static_cast<uint8_t>(m2_position * 255); // position in units
        float m3_position = m3.getPosition();
        usb_task._usb->getPayloadPointer()[3] = static_cast<uint8_t>(m3_position * 255); // position in units
        float m4_position = m4.getPosition();
        usb_task._usb->getPayloadPointer()[4] = static_cast<uint8_t>(m4_position * 255); // position in units

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
