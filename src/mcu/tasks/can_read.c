#include "../motor_h300/h300.h"
#include "esp_log.h"
#include "tasks.h"

// idk why I'm passing it in as a param
void read_can_data() {

    twai_message_t temp_msg;

    ESP_LOGI(__func__, "Entering Read Can Data");

    // this is the main task that runs always
    // and gets pre-empted into higher-priority tasks
    while (1) {
        twai_receive_v2(motor_control_bus, &temp_msg, portMAX_DELAY);

        if (temp_msg.extd && IS_H300_ID(temp_msg.identifier)) {
            if (xQueueSend(h300_rx_queue_handle, &temp_msg, portMAX_DELAY) != pdPASS) {
                // TODO: print error message
            }
        } else {
            // TODO: implement else case
        }
    }
}

void start_can_read_task() {
    static StackType_t read_can_data_stack[DEFAULT_STACK_SIZE];
    static StaticTask_t read_can_data_buffer;

    xTaskCreateStaticPinnedToCore(read_can_data, "read_can_data", DEFAULT_STACK_SIZE, NULL, READ_TASK_PRIO,
                                  read_can_data_stack, &read_can_data_buffer, 0);
}
