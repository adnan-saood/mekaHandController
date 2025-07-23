idf_component_register(SRCS "src/usb_driver.cpp"
    PRIV_REQUIRES  esp_driver_gpio  esp_tinyusb
    INCLUDE_DIRS "include" "../../include")
