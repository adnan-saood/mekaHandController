idf_component_register(SRCS "main.cpp" 
                #     "tasks/logger_task.cpp"
                #     "tasks/motor_task.cpp"
                    "tasks/sensor_task.cpp"
                #     "tasks/usb_task.cpp"
                    REQUIRES 
                                usb_driver
                                motor_driver 
                                sensor_driver
                    PRIV_REQUIRES 
                            esp_driver_ledc 
                            esp_driver_gpio
                            esp_driver_mcpwm
                            esp_driver_pcnt
                            esp_adc
                            tinyusb
                            esp_timer
                            esp_driver_uart
                    
                    INCLUDE_DIRS "." "tasks")
