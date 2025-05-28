/**
 * This declares tasks and memory required for tasks
 * (queues, mutexes, io etc)
 */

#ifndef TASKS_H
#define TASKS_H

#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#define DEFAULT_STACK_SIZE 10000

// === CAN static mem === //
extern twai_handle_t motor_control_bus;
extern twai_handle_t general_control_bus; // TODO: unused at the moment

#define H300_RX_QUEUE_LENGTH 20
#define H300_RX_ITEM_SIZE sizeof(twai_message_t)
extern QueueHandle_t h300_rx_queue_handle;

// CORE 0 TASKS

// background task

#define READ_TASK_PRIO 10 // low priority
void read_can_data(void *vsr_void);

#define HANDLE_H300_PRIO 15 // higher priority
// runs whenever we get h300 data
void handle_h300(void *vsr_void);

#define SEND_MOTOR_DATA_PRIO 20 // higher priority than all
// runs at 200 Hz
void send_motor_data(void *vsr_void);

#endif