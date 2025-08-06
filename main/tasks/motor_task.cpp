#include "motor_task.hpp"

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}



PIDFFController MotorTask::controllers[NUM_MOTORS];
PIDFFController MotorTask::motor[NUM_MOTORS];

void MotorTask::start()
{
    xTaskCreate(&MotorTask::taskFunction, taskName, stackSize, nullptr, priority, nullptr);
}

void MotorTask::taskFunction(void *pvParameters)
{
    // Initialize motor controllers with default gains
    // @TODO: Set gains from USB or initial values per motor
    for (int i = 0; i < NUM_MOTORS; ++i)
    {
        controllers[i].setGains(1.0f, 0.0f, 0.0f, 0.0f); // Default gains
    }
    while (true)
    {
        controlLoop();
        vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz control loop
    }
}

void MotorTask::controlLoop()
{
    // Get position and velocity commands from USB
    float commanded_positions[5];
    float commanded_stiffness[5];

    if (usbDriver != nullptr)
    {
        for (int i = 0; i < NUM_MOTORS; ++i)
        {
            commanded_positions[i] = static_cast<float>(usbDriver->getCommandedPosition(i)) / 255.0f;
            commanded_stiffness[i] = static_cast<float>(usbDriver->getCommandedStiffness(i)) / 255.0f;
        }
    }
    else
    {
        ESP_LOGE("MotorTask", "USB driver not set, cannot control motors.");
        return;
    }

    for (int i = 0; i < NUM_MOTORS; ++i)
    {
        float control_signal = controllers[i]->compute(
            commanded_positions[i],
            motor[i]->getCurrentPosition(), // Assuming MotorDriver has a method to get current position
            motor[i]->getCurrentVelocity()  // Assuming MotorDriver has a method to get current velocity
        );

        // Set the motor speed based on the control signal
        motor[i]->setSpeed(control_signal);
    }
}


void MotorTask::initMotorDrivers()
{

    ESP_LOGI("MotorTask", "Initializing motor drivers...");
    thumbPanMotor = new MotorDriver(0, MOTOR_J0_PWM, MOTOR_J0_DIR);
    thumbMotor = new MotorDriver(0, MOTOR_J1_PWM_H, MOTOR_J1_PWM_L);
    indexMotor = new MotorDriver(0, MOTOR_J2_PWM_H, MOTOR_J2_PWM_L);
    middleMotor = new MotorDriver(1, MOTOR_J3_PWM_H, MOTOR_J3_PWM_L);
    pinkyMotor = new MotorDriver(1, MOTOR_J4_PWM_H, MOTOR_J4_PWM_L);

    thumbMotor->init();
    thumbPanMotor->init();
    indexMotor->init();
    middleMotor->init();
    pinkyMotor->init();

    thumbMotor->setSpeed(0.0f);
    thumbPanMotor->setSpeed(0.0f);
    indexMotor->setSpeed(0.0f);
    middleMotor->setSpeed(0.0f);
    pinkyMotor->setSpeed(0.0f);
    ESP_LOGI("MotorTask", "Motor drivers initialized successfully.");


    ESP_LOGI("MotorTask", "PID controllers Initializing.");
    thumbPanController.setGains(1.0f, 0.0f, 0.0f, 0.0f);
    thumbController.setGains(1.0f, 0.0f, 0.0f, 0.0f);
    indexController.setGains(1.0f, 0.0f, 0.0f, 0.0f);
    middleController.setGains(1.0f, 0.0f, 0.0f, 0.0f);
    pinkyController.setGains(1.0f, 0.0f, 0.0f, 0.0f);
    ESP_LOGI("MotorTask", "PID controllers initialized successfully.");

}