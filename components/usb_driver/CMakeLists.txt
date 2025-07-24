idf_component_register(SRCS "src/usb_driver.cpp"
                            "src/usb_driver_callbacks.c"
                       PRIV_REQUIRES  esp_driver_gpio tinyusb esp_tinyusb freertos
                       INCLUDE_DIRS "include" "../../include")
