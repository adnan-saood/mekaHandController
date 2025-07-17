#include "logger_task.hpp"
#include <cstdio>

LoggerTask::LoggerTask(const char* name, uint32_t stackSize, UBaseType_t priority)
    : taskName(name), stackSize(stackSize), priority(priority) {}

void LoggerTask::start() {
    xTaskCreate(&LoggerTask::taskFunction, taskName, stackSize, nullptr, priority, nullptr);
}

void LoggerTask::taskFunction(void* pvParameters) {
    while (true) {
        printf("[Logger] Running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
