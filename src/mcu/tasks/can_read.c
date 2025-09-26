#include "tasks.h"
#include "../motor_h300/h300.h"


StackType_t read_can_data_stack[DEFAULT_STACK_SIZE];
StaticTask_t read_can_data_buffer;

// idk why I'm passing it in as a param
void read_can_data() {

    twai_message_t temp_msg;

    // this is the main task that runs always
    // and gets pre-empted into higher-priority tasks
    while (1) {
        twai_receive_v2(motor_control_bus, &temp_msg, portMAX_DELAY);

        if (temp_msg.extd && IS_H300_ID(temp_msg.identifier)) {
            if (xQueueSend(h300_rx_queue_handle, &temp_msg, portMAX_DELAY) !=
                pdPASS) {
                // TODO: print error message
            }
        } else {
            // TODO: implement else case
        }
    }
}

void start_can_read_task() {
    xTaskCreateStaticPinnedToCore(
        read_can_data, "read_can_data", DEFAULT_STACK_SIZE, NULL,
        READ_TASK_PRIO, read_can_data_stack, &read_can_data_buffer, 0);
}