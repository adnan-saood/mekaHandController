// MotorDriver.hpp

#ifndef MOTOR_DRIVER_HPP
#define MOTOR_DRIVER_HPP

extern "C"
{
#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

class MotorDriver
{
public:
    /**
     * @brief Construct a new MotorDriver instance.
     *
     * @param motor_name Name of the motor
     * @param mcpwm_unit MCPWM unit (0 or 1 on ESP32-S3)
     * @param pwm_high_gpio GPIO for high-side PWM (gpio_num_t)
     * @param pwm_low_gpio GPIO for low-side PWM (gpio_num_t)
     * @param encoder_gpio GPIO for encoder (gpio_num_t)
     */
    MotorDriver(const char* motor_name, int mcpwm_unit, gpio_num_t pwm_high_gpio, gpio_num_t pwm_low_gpio, gpio_num_t encoder_gpio);

    /**
     * @brief Initialize the motor driver (MCPWM timer, operator, comparators, generators).
     *        Must be called before using setSpeed().
     *
     * @return esp_err_t ESP_OK on success, or error code.
     */
    virtual esp_err_t init();

    /**
     * @brief Set motor speed.
     *
     * @param speed Motor speed in range [-1.0, 1.0]. Negative = reverse, Positive = forward.
     * @return esp_err_t ESP_OK on success, or error code.
     */
    virtual esp_err_t setSpeed(float speed);

    virtual float getSpeed() const{ return pwm_speed_; }

    /**
     * @brief Set the Position object
     * 
     * @param position float [0,1] open to close.
     * @return esp_err_t 
     */
    virtual esp_err_t setPosition(float position)
    {
        if (position < 0 || position > 1)
        {
            return ESP_ERR_INVALID_ARG;
        }

        commanded_position_ = position;
        return ESP_OK;
    }

    virtual esp_err_t startPositionControl();

    virtual esp_err_t stopPositionControl();

    virtual esp_err_t disarm_motor();

    virtual float getPosition();

    uint32_t getEncoderPosition() const;

    esp_err_t setGains(float kp, float ki, float kd)
    {
        this->kp = kp;
        this->ki = ki;
        this->kd = kd;
        return ESP_OK;
    }

protected:
    const char* motor_name_;
    const int mcpwm_unit_;
    const gpio_num_t pwm_high_gpio_;
    const gpio_num_t pwm_low_gpio_;
    const gpio_num_t encoder_gpio_;
    volatile uint32_t ma3_pulse_width_ = 0; // Latest captured pulse width
    float position_ = 0.0f;
    float commanded_position_ = 0.0f;
    float pwm_speed_ = 0.0f;

    const float open_position_ = 1000.0f; // need to load these 
    const float closed_position_ = 3000.0f; // need to load these

    mcpwm_timer_handle_t timer_ = nullptr;
    mcpwm_oper_handle_t operator_ = nullptr;
    mcpwm_cmpr_handle_t comparator_high_ = nullptr;
    mcpwm_cmpr_handle_t comparator_low_ = nullptr;
    mcpwm_gen_handle_t generator_high_ = nullptr;
    mcpwm_gen_handle_t generator_low_ = nullptr;

    bool initialized_ = false;
    
    static bool encoder_callback(mcpwm_cap_channel_handle_t cap_chan,
                             const mcpwm_capture_event_data_t *edata,
                             void *user_data);



    static void control_loop(void* pvParameters);
    // create a handle for the control task
    TaskHandle_t control_task_handle_;

    esp_err_t init_encoder();

    uint32_t pulse_array_[11] = {0};
    int pulse_index_ = 0;
    uint32_t cap_val_rise_ = 0;

    float kp = -1.0f;
    float ki = 0.0f;
    float kd = 0.0f;
};



class MotorDriverBLDC : public MotorDriver
{
public:
    /**
     * @brief Construct a new MotorDriverBLDC instance.
     * @param mcpwm_unit MCPWM unit (0 or 1 on ESP32-S3)
     * @param pwm_high_gpio GPIO for high-side PWM (gpio_num_t)
     *
     * @param pwm_low_gpio GPIO for low-side PWM (gpio_num_t)
     * @return MotorDriverBLDC instance.
     * */
    MotorDriverBLDC(const char* motor_name, int mcpwm_unit, gpio_num_t pwm, gpio_num_t dir, gpio_num_t encoder)
        : MotorDriver(motor_name, mcpwm_unit, pwm, dir, encoder) {}

    /**
     * @brief Initialize the BLDC motor driver.
     * @return esp_err_t ESP_OK on success, or error code.
     * */
    esp_err_t init() override
    {
        // Call base class init
        esp_err_t err = MotorDriver::init();
        if (err != ESP_OK)
        {
            return err;
        }
        // Additional BLDC-specific initialization can go here
        return ESP_OK;
    }

    /**
     * @brief Set BLDC motor speed.
     * @param speed Motor speed in range [-1.0, 1.0]. Negative = reverse, Positive = forward.
     * @return esp_err_t ESP_OK on success, or error code.
     * */
    esp_err_t setSpeed(float speed) override;


}
;

#endif // MOTOR_DRIVER_HPP
