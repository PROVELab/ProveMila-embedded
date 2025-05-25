#include "vsr.h"

// setup introspection here
extern vehicle_status_reg_s vsr;

// initialize the introspection array in the global vsr
vehicle_status_reg_s vsr = {.introspection_arr = {
#define APP(type, name)                                                        \
    {.offset = (uint16_t)offsetof(vehicle_status_reg_s, name),                 \
     .size = (uint8_t)sizeof(type)},
                                VSR_ITEMS
#undef APP
                            }};

#define INTROSPECTION_SIZE                                                     \
    (sizeof(vsr.introspection_arr) / sizeof(vsr_introspection_s))

// a spot to allocate the static mutices
StaticSemaphore_t vsr_mutexes[INTROSPECTION_SIZE];

int vsr_init(vehicle_status_reg_s *vsr) {
    for (int i = 0; i < INTROSPECTION_SIZE; i++) {
        );
    }
    return 0;
}