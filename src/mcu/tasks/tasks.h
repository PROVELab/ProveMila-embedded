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

#include "../vsr.h" // vehicle status register, holds all the information about the vehicle
#include "driver/twai.h"

#include "freertos/FreeRTOS.h"
#include "pecan/pecan.h"
#define DEFAULT_STACK_SIZE 10000

// CORE 0 TASKS

// ======= SEND MOTOR DATA TASK ======= //
// sends motor data at 200 Hz
#define SEND_MOTOR_DATA_PRIO 20 // higher priority than all
void start_send_motor_task();

// Console!
// read/write from console (uart)
#define CONSOLE_TASK_PRIO 11 // Mostly low priority but
void start_console_task();

// Logging
#define LOGGING_TASK_PRIO 11 // Similar priority to ~console
void start_logging_task();

#endif
