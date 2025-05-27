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

twai_handle_t motor_control_bus;
twai_handle_t general_control_bus;

void app_main() {
    // Initialize the global VSR
    vsr_init(&vehicle_status_register);

    //=== Initialize CAN stuff === //
    // TODO: modify tx/rx pins
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_0, GPIO_NUM_1, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    g_config.controller_id = 0; // this is the motor_control_bus

    // Install driver for TWAI bus 0
    g_config.controller_id = 0;
    if (twai_driver_install_v2(&g_config, &t_config, &f_config,
                               &motor_control_bus) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }


    // setup our tasks to run
    //
}