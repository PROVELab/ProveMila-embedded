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
twai_handle_t motor_control_bus; // externed, but initialized here
// twai_handle_t general_control_bus; // TODO: unused at the moment

// === Queue stuff (initialized here) === //
QueueHandle_t h300_rx_queue_handle;                                        // this is externed
static StaticQueue_t h300_rx_queue_buf;                                    // The actual Queue structure itself
static uint8_t rx_queue_databuf[H300_RX_QUEUE_LENGTH * H300_RX_ITEM_SIZE]; // the Actual storage of the queue

void start_twai(void) {
    static const char* TAG = __func__;
    // Use safe pins – avoid 0/1/3 and strap pins
    const gpio_num_t TX_PIN = GPIO_NUM_33;
    const gpio_num_t RX_PIN = GPIO_NUM_32;

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    // The v2 API requires controller_id on chips that have >1 controller; ESP32 classic = 0
    g_config.controller_id = 0;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    // twai_timing_config_t t_config = T();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install_v2(&g_config, &t_config, &f_config, &motor_control_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install_v2 failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "TWAI driver installed");

    err = twai_start_v2(motor_control_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start_v2 failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "TWAI started");
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

    addParam(&plpc, process_motor_fxn_code1);
    addParam(&plpc, process_motor_fxn_code2);

    pedal_main(&plpc);
    ESP_LOGI(__func__, "Initializing VSR, finished TWAI");

    twai_message_t msg;
    msg.identifier = msg.data_length_code = 0;

    // setup the queue for receiving h300 messages (up to 20 individual messages
    // are can be buffered here)
    h300_rx_queue_handle =
        xQueueCreateStatic(H300_RX_QUEUE_LENGTH, H300_RX_ITEM_SIZE, rx_queue_databuf, &h300_rx_queue_buf);

    // Core 0 tasks
    // start_can_read_task(); // low priority, always running in background
    ESP_LOGI(__func__, "Started CAN Read Task");

    // start_handle_h300_task(); // higher priority, runs when we get h300 data
    start_console_task();
    ESP_LOGI(__func__, "StartedConsole");

    // // Send data to the motor task
    start_handle_h300_task();
    start_send_motor_task();
    // while (true) {
    //     vTaskDelay(pdMS_TO_TICKS(5000));
    //     ESP_LOGI("MAIN", "Still alive");
    // }
}
