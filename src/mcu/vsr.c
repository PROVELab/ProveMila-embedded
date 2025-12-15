#include "vsr.h"

// Initialize the vehicle status register:
volatile vehicle_status_reg_s vehicle_status_register = {};

int vsr_init(vehicle_status_reg_s* vsr) {

    // initialize the mutexes, and zero out the structs
#define APP(type, name)                                                 \
    static StaticSemaphore_t name##_mutex_buf;                          \
    vsr->name##_mutex = xSemaphoreCreateMutexStatic(&name##_mutex_buf); \
    memset((void*) &vsr->name, 0, sizeof(type));

    // This ends up being something like:
    //   static StaticSemaphore_t motor_power_mutex_buf;
    //   vsr->motor_power_mutex =
    //   xSemaphoreCreateMutexStatic(&motor_power_mutex_buf);
    // ...
    VSR_ITEMS
#undef APP

    vsr->pedal.use_pedal = true; // default to using the pedal

    return 0;
}
