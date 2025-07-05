#ifndef SEA_MOTOR_CONTROLLER_H
#define SEA_MOTOR_CONTROLLER_H

#include "esp_err.h"

#include "driver/mcpwm_timer.h"

#include "driver/ledc.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Add these at the top of your motor_control.cpp:
#include "driver/mcpwm_prelude.h"   // For MCPWM new driver API

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h" // For ADC calibration schemes
#include "esp_adc/adc_continuous.h" // For ADC continuous driver
#include "esp_adc/adc_filter.h" // For ADC calibration schemes
#include "esp_adc/adc_monitor.h" // For ADC calibration schemes
#include "esp_adc/adc_oneshot.h"    // For ADC oneshot driver



#include <cmath> // For fabs
#include <algorithm> // For std::clamp


#define NUM_MOTORS                5 // Total motors
#define NUM_HBRIDGE_MOTORS        4 // Motors 1-4 using H-bridge (MCPWM)
#define NUM_TORQUE_DOT_SAMPLES    8 // From original PIC code, for derivative term


// IMPORTANT: ESP-IDF Version is 5.4.3 Stable Release



#endif // SEA_MOTOR_CONTROLLER_H