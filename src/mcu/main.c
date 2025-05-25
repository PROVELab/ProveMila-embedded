#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../espMutexes/mutex_declarations.h" //sets uo static mutexes. To add another mutex, declare it in this file, and its .c file, and increment mutexCount
#include "../pecan/pecan.h"                   //helper code for CAN stuff

#include "vsr.h" // vehicle status register, holds all the information about the vehicle

static void twai_init() {
    // TODO: initialize the CAN driver here
}

void app_main() {
    // Initialize the global VSR
    vsr_init(&vehicle_status_register);
}