idf_component_register(SRCS "main.cpp" 
                    "tasks/logger_task.cpp"
                    "tasks/motor_task.cpp"
                    "tasks/sensor_task.cpp"
                    "tasks/usb_task.cpp"
                    REQUIRES motor_driver sensor_driver usb_driver 
                    PRIV_REQUIRES 
                            esp_driver_ledc 
                            esp_driver_gpio
                            esp_driver_mcpwm
                            esp_driver_pcnt
                            esp_adc
                            tinyusb
                    
                    INCLUDE_DIRS "." "tasks")
