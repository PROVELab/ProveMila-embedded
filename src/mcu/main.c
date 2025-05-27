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

#include "h300.h"
#include "vsr.h" // vehicle status register, holds all the information about the vehicle

#define DEFAULT_STACK_SIZE 10000

// === CAN static mem === //
twai_handle_t motor_control_bus;
twai_handle_t general_control_bus; // TODO: unused at the moment

// === Task initialization static mem === //
StackType_t read_can_data_stack[DEFAULT_STACK_SIZE];

StaticTask_t read_can_data_buffer;

// idk why I'm passing it in as a param
void read_can_data(void *vsr_void /*idk why I did it this way. TODO: fix */) {
    // get ourselves a nice vsr
    volatile vehicle_status_reg_s *vsr =
        (volatile vehicle_status_reg_s *)vsr_void;
}

// The code sends the current
// motor setpoint in RPM (via motor_control_s)
// and sends it at 200 Hz
void send_motor_data(void *vsr_void /*idk why I did it this way*/) {
    // convert to normal
    volatile vehicle_status_reg_s *vsr =
        (volatile vehicle_status_reg_s *)vsr_void;

    // Initialize and set frequency to 200 Hz
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(5);

    // infinite loop send
    while (1) {

        // get this stuff local quickly and then
        // send with CAN later (so we don't hold the lock too long)
        // and send the message after semaphore
        int32_t speed_reference;
        uint8_t discharge_limit_pct;
        uint8_t charge_limit_pct;

        // acquire the lock
        ACQ_SUB_REG_INF(motor_control, {
            speed_reference = vsr->motor_control.speed_reference;
            discharge_limit_pct = vsr->motor_control.discharge_limit_pct;
            charge_limit_pct = vsr->motor_control.charge_limit_pct;
        })

        // send the message as a twai

        // === Quick error checking ===
        high_level_motor_state ms; // should we stop or send
        ACQ_SUB_REG_INF(motor_error, ms = vsr->motor_error.motor_state;)

        // immediately set up for 0 RPM, stopping the motor essentially
        twai_message_t msg = {// extended id (2.0B)
                              .extd = 1,
                              // disable everything else
                              .rtr = 0,
                              .ss = 0,
                              .self = 0,
                              .dlc_non_comp = 0,
                              // set the identifier to expected one
                              .identifier = REFERENCE_MSG_ID

        };

        // setup things
        if (ms == MOTOR_ERROR_STOP) {
            // set discharge limit, and charge limit to 0
            // sets to neutral mode: // TODO VERIFY THIS
            uint8_t pwm_ref_sel = 0; // pwm off, reference selection: neutral
            uint16_t reference = 0;  // not relevant in neutral mode probably
            uint8_t zero_discharge_limit_pct =
                100;                            // TODO: verify this (100==0%?)
            uint8_t zero_regen_limit_pct = 100; // TODO: verify this (100==0%?)

            // copy over things
            memcpy(msg.data, &pwm_ref_sel, 1);
            memcpy(&msg.data[1], &reference, 2);
            memcpy(&msg.data[3], &zero_discharge_limit_pct, 1);
            memcpy(&msg.data[4], &zero_regen_limit_pct, 1);

        } else {
            // use the actual requested data
            uint8_t pwm_ref_sel = 0b00000100; // PWM off, but speed reference
            uint16_t reference_speed_8ths =
                speed_reference / 8; // since it needs to be in 0.125 RPM

            memcpy(msg.data, &pwm_ref_sel, 1);
            memcpy(&msg.data[1], &reference_speed_8ths, 2);
            memcpy(&msg.data[3], &discharge_limit_pct, 1);
            memcpy(&msg.data[4], &charge_limit_pct, 1);
        }

        msg.data_length_code = 5; // size is always 5 regardless

        if (twai_transmit_v2(motor_control_bus, &msg, portMAX_DELAY) !=
            ESP_OK) {
            printf("Failed to transmit CAN message from send_motor\n");
            return;
        }

        // wait to get to 200 Hz
        xTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

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

    // TODO: start the twai driver

    // === Schedule Tasks === //
    // motor controller tasks start on
    xTaskCreateStaticPinnedToCore(
        read_can_data, // function called
        "read_mcan", DEFAULT_STACK_SIZE,
        (void *)&vehicle_status_register, // unused

        5, // semi-low priority. this is the usually active task
        // static stuff:
        read_can_data_stack, &read_can_data_buffer,
        0 // core 1
    );

    xPortStartScheduler();
    vTaskStartScheduler();
}