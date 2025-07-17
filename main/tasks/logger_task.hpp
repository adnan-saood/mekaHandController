#pragma once

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

class LoggerTask {
public:
    LoggerTask(const char* name = "Logger", uint32_t stackSize = 2048, UBaseType_t priority = 2);
    void start();

private:
    static void taskFunction(void* pvParameters);
    const char* taskName;
    uint32_t stackSize;
    UBaseType_t priority;
};
