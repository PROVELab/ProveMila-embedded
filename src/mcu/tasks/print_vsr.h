#ifndef PRINT_VSR_H
#define PRINT_VSR_H

#include "../vsr.h"
#include <stdbool.h>
#include <stddef.h>

typedef void (*vsr_print_function)(volatile vehicle_status_reg_s* vsr);

typedef struct {
    const char* name;
    vsr_print_function print;
} vsr_topic_printer_t;

const vsr_topic_printer_t* vsr_find_topic_printer(const char* name);
void vsr_print_topic(const vsr_topic_printer_t* topic, volatile vehicle_status_reg_s* vsr);
void vsr_print_all_topics(volatile vehicle_status_reg_s* vsr);
void vsr_print_available_topics(void);

#endif
