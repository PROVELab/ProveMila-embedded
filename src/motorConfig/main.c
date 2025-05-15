#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../espMutexes/mutex_declarations.h" //sets uo static mutexes. To add another mutex, declare it in this file, and its .c file, and increment mutexCount
#include "../pecan/pecan.h"                   //helper code for CAN stuff
// tracing functionality to give warning for free or alloc.
#include "../pecan/espBusRestart.h"
#include "esp_heap_caps.h"

int32_t checkBus_myId = 20; // pass this as a parameter for checkCanHandler
#define STACK_SIZE 20000    // for deciding stack size
StaticTask_t
    checkBus_Buffer; // task for checkBus status, and restarting when necessary
StackType_t checkBus_Stack[STACK_SIZE];
StaticTask_t
    checkUART_Buffer; // task for checkBus status, and restarting when necessary
StackType_t checkUART_Stack[STACK_SIZE];

int init = 0; // indicates that mutexes have been initialized, so we may print a
              // warning for allocations and frees
void esp_heap_trace_alloc_hook(
    void *ptr, size_t size,
    uint32_t caps) { // is called every time memory is allocated, feature
                     // enabled in menuconfig
    if (init) {
        mutexPrint("Warning, allocating memory!\n");
    }
}
void esp_heap_trace_free_hook(void *ptr) {
    if (init) {
        mutexPrint("Warning, freeing memory!\n");
    }
}
void enableMotor() { mutexPrint("enabling Motor\n"); }

void disableMotor() { mutexPrint("disabling Motor\n"); }

void checkUartMSG(void *pvParameters) { // should send all this to telem
    char input[10];
    int motorOn = 0;

    for (;;) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (input[0] == 'r' && !motorOn) {
                enableMotor();
                motorOn = 1;
            } else if (input[0] == 'o' && motorOn) {
                disableMotor();
                motorOn = 0;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {

    // Initialize configuration structures using macro initializers
    // a hasty testing of each pin found each numbered PIN un the board (0-35)
    // worked for TWAI for each number that appears on the board, except for pin
    // 34, and 35, which I don't beleive are actually GPIO pins, since the
    // datasheet says the MCU has only 34 pins, so having a pin 35 wouldnt make
    // much sense.
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        GPIO_NUM_33, GPIO_NUM_32,
        TWAI_MODE_NORMAL); // TWAI_MODE_NORMAL for standard behavior
    // twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33,
    // GPIO_NUM_32, TWAI_MODE_LISTEN_ONLY); //TWAI_MODE_NORMAL for standard
    // behavior
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // Install TWAI driver

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        // return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        // return;
    }

    mutexInit(); // initialize mutexes
    init = 1;

    int16_t err;
    if ((err = sendStatusUpdate(canRecoveryFlag, vitalsID))) {
        char buffer[50];
        sprintf(buffer, "error sending CAN recovery Msg: %d\n",
                err); // Convert the int8_t to a string
        mutexPrint(buffer);
    }
    TaskHandle_t checkUARTHandler =
        xTaskCreateStaticPinnedToCore( // prints out bus status info
            checkUartMSG,              /* Function that implements the task. */
            "recUart",                 /* Text name for the task. */
            STACK_SIZE, /* Number of indexes in the xStack array. */
            (void *)1,
            /* Parameter passed into the task. */ // should only use constants
                                                  // here. Global variables may
                                                  // be ok? cant be a stack
                                                  // variable.
            1,                 /* Priority at which the task is created. */
            checkUART_Stack,   /* Array to use as the task's stack. */
            &checkUART_Buffer, /* Variable to hold the task's data structure. */
            tskNO_AFFINITY);   // assigns printHello to core 0

    TaskHandle_t checkCanHandler =
        xTaskCreateStaticPinnedToCore( // prints out bus status info
            check_bus_status,          /* Function that implements the task. */
            "checkCan",                /* Text name for the task. */
            STACK_SIZE, /* Number of indexes in the xStack array. */
            (void *)&checkBus_myId,
            /* Parameter passed into the task. */ // should only use constants
                                                  // here. Global variables may
                                                  // be ok? cant be a stack
                                                  // variable.
            1,                /* Priority at which the task is created. */
            checkBus_Stack,   /* Array to use as the task's stack. */
            &checkBus_Buffer, /* Variable to hold the task's data structure. */
            tskNO_AFFINITY);  // assigns printHello to core 0
}