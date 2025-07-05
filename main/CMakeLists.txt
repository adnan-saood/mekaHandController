idf_component_register(SRCS "main.cpp" "motor_control.cpp" "motor_control.hpp"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_driver_pcnt esp_adc
                    INCLUDE_DIRS ".")
