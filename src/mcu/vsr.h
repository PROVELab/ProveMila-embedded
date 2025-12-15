/**
 * Vehicle State Register
 * - basically a big struct that holds
 *   all the information about the vehicle
 */
#ifndef VSR_H
#define VSR_H

#include <stdint.h> // for fixed size
#include <string.h>

#include "esp_timer.h" // for getting timestamps, since start
#include "freertos/FreeRTOS.h"
#include "motor_h300/vsr_motor.h" // motor speed status

// Acquire and release a vsr subregister semaphore
// Assumes vsr is defined as a pointer named 'vsr'
// and that you pass in the name of the struct item
// (to associate the item with the mutex). waits infinite time to acquire
// (but ideally FreeRTOS will give others time here)
// Use body to do whatever you want while you have the lock, ideally copying
// out items from vsr->mutating_element
// Also updates the timestamp for that element
#define ACQ_REL_VSRSEM_W(mutating_element, body)                  \
    xSemaphoreTake(vsr->mutating_element##_mutex, portMAX_DELAY); \
    vsr->mutating_element##_timestamp = esp_timer_get_time();     \
    body xSemaphoreGive(vsr->mutating_element##_mutex);

// This one does the same as above, but doesn't set timestamp
// (so use it when reading)
#define ACQ_REL_VSRSEM_R(mutating_element, body)                  \
    xSemaphoreTake(vsr->mutating_element##_mutex, portMAX_DELAY); \
    body xSemaphoreGive(vsr->mutating_element##_mutex);

// X-Macro for defining parts of the vehicle status register
#define VSR_ITEMS                            \
    APP(motor_mspeed_status_s, motor_power)  \
    APP(motor_hspeed_status_s, motor_speed)  \
    APP(motor_safety_status_s, motor_safety) \
    APP(motor_control_s, motor_control)      \
    APP(motor_error_state, motor_error)      \
                                             \
    APP(pedal_s, pedal)                      \
                                             \
    APP(motor_protections_1_s, motor_prot1)  \
    APP(motor_protections_2_s, motor_prot2)

// === Full VSR definition ===
typedef struct {
    float pedal_supply_voltage; // in mV
    float pedal_position_pct;   // 0..100 %
    float pedal_raw_1;          // raw ADC value of pedal reading 1
    float pedal_raw_2;          // raw ADC value of pedal reading 2
    int32_t tx_value;           // value to transmit over CAN
    bool use_pedal;             // whether to use the pedal or not
} pedal_s;

typedef struct {
// add the actual fields of the VSR via apply, adding mutexes
#define APP(type, name)             \
    SemaphoreHandle_t name##_mutex; \
    int64_t name##_timestamp;       \
    type name;
    // END APP Macro

    // Apply macro to all items
    // so basically this ends up being something like:
    //   SemaphoreHandle_t motor_power_mutex;
    //   motor_mspeed_status_s motor_power;
    // ...
    VSR_ITEMS
#undef APP
} vehicle_status_reg_s;

extern volatile vehicle_status_reg_s vehicle_status_register;

// initializes the mutexes and the structs
// returns 0 on success, -1 on failure (but it always returns 0)
int vsr_init(vehicle_status_reg_s* vsr);

#endif
