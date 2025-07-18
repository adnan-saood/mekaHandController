#include "motor_driver.hpp"
#include <cstdio>  // Replace with real driver includes for PWM

MotorDriver::MotorDriver() {
    // Init PWM peripheral (ledc, MCPWM, etc.)
}

void MotorDriver::setPWM(float duty_cycle) {
    // Clamp value
    if (duty_cycle > 1.0f) duty_cycle = 1.0f;
    if (duty_cycle < -1.0f) duty_cycle = -1.0f;

    // Convert to hardware format and write
    printf("[PWM] Writing duty cycle: %.2f\n", duty_cycle);

    // TODO: use real ESP32 APIs to set duty cycle (e.g., ledc_set_duty, ledc_update_duty)
}
