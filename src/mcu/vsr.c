#include "vsr.h"

// Initialize the vehicle status register:
volatile vehicle_status_reg_s vehicle_status_register = {};

int vsr_init(volatile vehicle_status_reg_s *vsr) {

    // initialize the mutexes
#define APP(type, name)                                                        \
    static StaticSemaphore_t name##_mutex_buf;                                 \
    vsr->name##_mutex = xSemaphoreCreateMutexStatic(&name##_mutex_buf);

    // This ends up being something like:
    //   static StaticSemaphore_t motor_power_mutex_buf;
    //   vsr->motor_power_mutex = xSemaphoreCreateMutexStatic(&motor_power_mutex_buf);
    // ...
    VSR_ITEMS
#undef APP

    // zero out vsr for safety, no need for mutex
    // since nothing will be running when we call this
    memset(&vehicle_status_register, 0, sizeof(vehicle_status_register));

    return 0;
}