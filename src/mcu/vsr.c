#include "vsr.h"

// setup introspection here
extern vehicle_status_reg_s vehicle_status_register;

// initialize the introspection array in the global vsr
vehicle_status_reg_s vehicle_status_register = {};

int vsr_init(vehicle_status_reg_s *vsr) {

    // initialize the mutexes
#define APP(type, name)                                                        \
    static StaticSemaphore_t name##_mutex_buf;                                 \
    vsr->name##_mutex = xSemaphoreCreateMutexStatic(&name##_mutex_buf);
    VSR_ITEMS
#undef APP

    return 0;
}