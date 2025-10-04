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
#include "../pecan/pecan.h" //helper code for CAN stuff

#include "motor_h300/h300.h"
#include "tasks/tasks.h"
#include "vsr.h" // vehicle status register, holds all the information about the vehicle

// === CAN static mem === //
twai_handle_t motor_control_bus; // externed, but initialized here
// twai_handle_t general_control_bus; // TODO: unused at the moment

// === Queue stuff (initialized here) === //
QueueHandle_t h300_rx_queue_handle;                                        // this is externed
static StaticQueue_t h300_rx_queue_buf;                                    // The actual Queue structure itself
static uint8_t rx_queue_databuf[H300_RX_QUEUE_LENGTH * H300_RX_ITEM_SIZE]; // the Actual storage of the queue

void start_twai() {
    //=== Initialize CAN stuff === //
    // TODO: modify tx/rx pins
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_0, GPIO_NUM_1, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    g_config.controller_id = 0; // this is the motor_control_bus

    // Install driver for TWAI bus 0
    g_config.controller_id = 0;
    if (twai_driver_install_v2(&g_config, &t_config, &f_config, &motor_control_bus) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    // TODO: start the twai driver
    if (twai_start_v2(motor_control_bus)) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }
}

void app_main() {
    // Initialize the global VSR
    vsr_init(&vehicle_status_register);

    start_twai(); // initialize can stuff

    // setup the queue for receiving h300 messages (up to 20 individual messages
    // are can be buffered here)
    h300_rx_queue_handle =
        xQueueCreateStatic(H300_RX_QUEUE_LENGTH, H300_RX_ITEM_SIZE, rx_queue_databuf, &h300_rx_queue_buf);

    // Core 0 tasks
    start_can_read_task(); // low priority, always running in background

    start_handle_h300_task(); // higher priority, runs when we get h300 data

    // Send data to the motor task
    start_send_motor_task();

    xPortStartScheduler();
    vTaskStartScheduler();
}
