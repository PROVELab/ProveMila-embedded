/**
 * This declares tasks and memory required for tasks
 * (queues, mutexes, io etc)
 * as well as functions to start tasks.
 * 
 * Tasks themselves, however, are defined in their own .c files
 * and usually not exposed.
 * 
 * Most of them are in tasks/ but motor-specific tasks are in motor_h300/
 */

#ifndef TASKS_H
#define TASKS_H

#include "driver/twai.h"
#include "../vsr.h" // vehicle status register, holds all the information about the vehicle

#include "freertos/FreeRTOS.h"
#define DEFAULT_STACK_SIZE 10000

// === CAN static mem === //
extern twai_handle_t motor_control_bus;
extern twai_handle_t general_control_bus; // TODO: unused at the moment


// CORE 0 TASKS

// background task

// Items shared by multiple tasks:
extern twai_handle_t motor_control_bus;

// ======== CAN READ TASK ======= //
// reads from CAN bus and puts onto various queues
#define READ_TASK_PRIO 10 // low priority
void start_can_read_task(); // starts the CAN read task

// CAN Read task puts onto these queues:
#define H300_RX_QUEUE_LENGTH 20
#define H300_RX_ITEM_SIZE sizeof(twai_message_t)
extern QueueHandle_t h300_rx_queue_handle; // Queue for receiving h300 messages

// ====== HANDLE H300 TASK ======= //
// pops off h300 queue and parses/handles the packet
#define HANDLE_H300_PRIO 15 // higher priority
void start_handle_h300_task(); // Starts the handle_h300 task

// ======= SEND MOTOR DATA TASK ======= //
// sends motor data at 200 Hz
#define SEND_MOTOR_DATA_PRIO 20 // higher priority than all
void start_send_motor_data_task(); // Starts the send_motor_data task

#endif