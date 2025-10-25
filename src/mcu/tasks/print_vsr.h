#ifndef PRINT_VSR_H
#define PRINT_VSR_H

#include "../vsr.h"
#include <stdbool.h>
#include <stddef.h>

typedef void (*vsr_topic_print_fn_t)(volatile vehicle_status_reg_s* vsr);

typedef struct {
    const char* name;
    vsr_topic_print_fn_t print;
} vsr_topic_printer_t;

const vsr_topic_printer_t* vsr_find_topic_printer(const char* name);
void vsr_print_topic(const vsr_topic_printer_t* topic, volatile vehicle_status_reg_s* vsr);
void vsr_print_available_topics(void);

#endif
