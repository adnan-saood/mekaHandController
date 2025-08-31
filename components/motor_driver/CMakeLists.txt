idf_component_register(SRCS "src/motor_driver.cpp"
                            "src/encoder_driver.cpp"
                            "src/bldc_motor_driver.cpp"   
                            "src/position_control.cpp"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_driver_pcnt esp_timer esp_driver_rmt
                    INCLUDE_DIRS "include" "../../include")
