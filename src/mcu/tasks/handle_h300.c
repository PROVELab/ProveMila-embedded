#include "tasks.h"
#include "../motor_h300/h300.h"

StackType_t handle_h300_data_stack[DEFAULT_STACK_SIZE];
StaticTask_t handle_h300_data_buffer;

// pop off queue and parse the packet/handle
void handle_h300() {

    twai_message_t temp_msg;
    while (1) {
        if (xQueueReceive(h300_rx_queue_handle, &temp_msg, portMAX_DELAY) !=
            pdPASS) {
            // TODO: handle Error here
        } else {
            // call parse_packet
            parse_packet(&temp_msg, &vehicle_status_register);
        }
    }
}

void start_handle_h300_task() {
    xTaskCreateStaticPinnedToCore(
        handle_h300, "handle_h300", DEFAULT_STACK_SIZE,
        NULL, HANDLE_H300_PRIO,
        handle_h300_data_stack, &handle_h300_data_buffer, 0);
}
