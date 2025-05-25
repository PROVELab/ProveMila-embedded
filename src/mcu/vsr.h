/**
 * Vehicle State Register
 * - basically a big struct that holds
 *   all the information about the vehicle
 */
#ifndef VSR_H
#define VSR_H

#include <stdint.h> // for fixed size

#include "freertos/FreeRTOS.h"
#include "vsr_motor.h" // motor speed status

// X-Macro for defining parts of the vehicle status register
#define VSR_ITEMS                                                              \
    APP(motor_power_status_s, motor_power)                                     \
    APP(motor_speed_status_s, motor_speed)                                     \
    APP(motor_safety_status_s, motor_safety)

// === Full VSR definition ===

// struct that helps introspect the vsr
typedef struct {
    uint16_t offset;
    uint8_t size; // size of the struct in bytes
} vsr_introspection_s;

typedef struct {
// add the actual fields of the VSR via apply, adding mutexes
#define APP(type, name)                                                        \
    SemaphoreHandle_t name##_mutex;                                            \
    type name;

    VSR_ITEMS
#undef APP

    // code introspection helper
    // #define APP(l, r) 1 +
    //     vsr_introspection_s introspection_arr[VSR_ITEMS 0];
    // #undef APP

    vsr_introspection_s introspection_arr[
#define APP(l, r) 1 +
        VSR_ITEMS 0
#undef APP
    ];
} vehicle_status_reg_s;

extern vehicle_status_reg_s vsr;

// initializes the mutexes and the structs
// returns 0 on success, -1 on failure
int vsr_init(vehicle_status_reg_s *vsr);

#endif
