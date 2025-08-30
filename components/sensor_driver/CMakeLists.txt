idf_component_register(SRCS "src/adc.cpp"
    "src/imu.cpp"
    "src/encoder.cpp"
    PRIV_REQUIRES esp_driver_gpio esp_driver_pcnt esp_driver_spi esp_driver_i2c
    INCLUDE_DIRS "include" "../../include")
