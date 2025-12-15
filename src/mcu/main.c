#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../pecan/pecan.h" //helper code for CAN stuff

#include "esp_log.h"
#include "motor_h300/h300.h"
#include "tasks/tasks.h"
#include "vsr.h" // vehicle status register, holds all the information about the vehicle

#include "sensors/pedalSensor/pedal_sensor.h" // for pedal reading

// === CAN static mem === //

void setup_motor_controller_params(PCANListenParamsCollection* plpc) {
    CANListenParam process_motor_fxn_code1 = {
        .listen_id = combinedID(1, 0),
        .handler = parse_packet_motor,
        .mt = MATCH_FUNCTION,
    };

    CANListenParam process_motor_fxn_code2 = {
        .listen_id = combinedID(0, 0),
        .handler = parse_packet_motor,
        .mt = MATCH_FUNCTION,
    };

    addParam(plpc, process_motor_fxn_code1);
    addParam(plpc, process_motor_fxn_code2);
}

void app_main() {
    ESP_LOGI(__func__, "Hello, minimal app starting");

    // Initialize the global VSR
    vsr_init(&vehicle_status_register);

    static PCANListenParamsCollection plpc = {
        .arr = {{0}},
        .defaultHandler = defaultPacketRecv,
        .size = 0,
    };

    setup_motor_controller_params(&plpc); // setup motor controller CAN handlers
    pedal_main(&plpc); // start the pedal sensor reading task, which will also add its CAN handlers and start PECAN

    ESP_LOGI(__func__, "Initialized VSR, finished TWAI config");

    // Core 0 tasks
    start_console_task();
    ESP_LOGI(__func__, "StartedConsole");

    start_logging_task(); //<- replaced with SD card version.
    ESP_LOGI(__func__, "StartedLogging");

    // Send data to the motor task
    start_send_motor_task();
    ESP_LOGI(__func__, "Started motor tasks");
}
