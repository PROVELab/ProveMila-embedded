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
    APP(motor_mspeed_status_s, motor_power)                                    \
    APP(motor_hspeed_status_s, motor_speed)                                    \
    APP(motor_safety_status_s, motor_safety)

// === Full VSR definition ===

typedef struct {
// add the actual fields of the VSR via apply, adding mutexes
#define APP(type, name)                                                        \
    SemaphoreHandle_t name##_mutex;                                            \
    type name;

    VSR_ITEMS
#undef APP
} vehicle_status_reg_s;

extern vehicle_status_reg_s vehicle_status_register;

// initializes the mutexes and the structs
// returns 0 on success, -1 on failure
int vsr_init(vehicle_status_reg_s *vsr);

#endif
