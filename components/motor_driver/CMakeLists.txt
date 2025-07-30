idf_component_register(SRCS "src/motor_driver.cpp"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_driver_mcpwm esp_driver_pcnt 
                    INCLUDE_DIRS "include")
