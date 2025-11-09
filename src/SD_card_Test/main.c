
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#include "tasks.h"
void app_main() {
    start_logging_task();    //<- replaced with SD card version.
    ESP_LOGI(__func__, "StartedLogging");
}