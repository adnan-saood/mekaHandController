idf_component_register(SRCS "main.cpp" 
                    "tasks/logger_task.cpp"
                    "tasks/motor_task.cpp"
                    "tasks/sensor_task.cpp"
                    "tasks/usb_task.cpp"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_driver_pcnt esp_adc
                    INCLUDE_DIRS "." "tasks")
