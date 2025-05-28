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

#include "motor_h300/h300.h"
#include "tasks.h"
#include "vsr.h" // vehicle status register, holds all the information about the vehicle

// === CAN static mem === //
twai_handle_t motor_control_bus;
twai_handle_t general_control_bus; // TODO: unused at the moment

// === Task initialization static mem === //
StackType_t read_can_data_stack[DEFAULT_STACK_SIZE];
StackType_t send_motor_data_stack[DEFAULT_STACK_SIZE];
StackType_t handle_h300_data_stack[DEFAULT_STACK_SIZE];

StaticTask_t read_can_data_buffer;
StaticTask_t send_motor_data_buffer;
StaticTask_t handle_h300_data_buffer;

// === Queue stuff === //
QueueHandle_t h300_rx_queue_handle; // this is externed
uint8_t rx_queue_databuf[H300_RX_QUEUE_LENGTH * H300_RX_ITEM_SIZE];
StaticQueue_t h300_rx_queue_buf;

// idk why I'm passing it in as a param
void read_can_data(void *arg) {

    twai_message_t temp_msg;

    // this is the main task that runs always
    // and gets pre-empted into higher-priority tasks
    while (1) {
        twai_receive_v2(motor_control_bus, &temp_msg, portMAX_DELAY);

        if (temp_msg.extd && IS_H300_ID(temp_msg.identifier)) {
            if (xQueueSend(h300_rx_queue_handle, &temp_msg, portMAX_DELAY) !=
                pdPASS) {
                // TODO: print error message
            }
        } else {
            // TODO: implement else case
        }
    }
}

// pop off queue and parse the packet/handle
void handle_h300(void *vsr_void) {
    volatile vehicle_status_reg_s *vsr =
        (volatile vehicle_status_reg_s *)vsr_void;

    twai_message_t temp_msg;
    while (1) {
        if (xQueueReceive(h300_rx_queue_handle, &temp_msg, portMAX_DELAY) !=
            pdPASS) {
            // TODO: handle Error here
        } else {
            // call parse_packet
            parse_packet(&temp_msg, vsr);
        }
    }
}

void start_twai() {
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

    // setup the queue
    h300_rx_queue_handle =
        xQueueCreateStatic(H300_RX_QUEUE_LENGTH, H300_RX_ITEM_SIZE,
                           rx_queue_databuf, &h300_rx_queue_buf);

    // === Schedule Tasks === //
    // read general can data (from the motor controller bus)
    xTaskCreateStaticPinnedToCore(
        read_can_data, "read_mcan", DEFAULT_STACK_SIZE,
        (void *)&vehicle_status_register, // unused
        // semi-low priority. this is the usually active task
        READ_TASK_PRIO,
        // static stuff:
        read_can_data_stack, &read_can_data_buffer,
        0 // core 1
    );

    // send motor data at 200 Hz; Higher priority since it needs to be done!
    xTaskCreateStaticPinnedToCore(
        send_motor_data, "send_motor_data", DEFAULT_STACK_SIZE,
        (void *)&vehicle_status_register, SEND_MOTOR_DATA_PRIO,
        send_motor_data_stack, &send_motor_data_buffer, 0);

    // handle messages from the motor controller; lower priority than
    // sending data, but higher than reading; it just polls the queue
    // and calls handler functions (check motor_h300) when necessary
    xTaskCreateStaticPinnedToCore(
        handle_h300, "handle_h300", DEFAULT_STACK_SIZE,
        (void *)&vehicle_status_register, HANDLE_H300_PRIO,
        handle_h300_data_stack, &handle_h300_data_buffer, 0);

    xPortStartScheduler();
    vTaskStartScheduler();
}