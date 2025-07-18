
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"

#include "tasks/logger_task.hpp"
extern "C" void app_main(void)
{
    //No operation.
    printf("Motor Control Example\n");

    LoggerTask loggerTask;
    loggerTask.start();

}