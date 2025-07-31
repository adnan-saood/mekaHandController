#pragma once
#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"

class MotorDriver {
public:
    MotorDriver(gpio_num_t pwmA, gpio_num_t pwmB, int mcpwm_group);
    void init();
    void setSpeed(float speed);

private:
    gpio_num_t gpio_pwmA, gpio_pwmB;
    int mcpwm_group;
    mcpwm_timer_handle_t timer_handle;
    mcpwm_oper_handle_t opA_handle, opB_handle;
    mcpwm_cmpr_handle_t pwmA_cmpr, pwmB_cmpr;
    mcpwm_gen_handle_t genA_handle, genB_handle;
};