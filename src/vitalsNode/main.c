#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../espBase/debug_esp.h" //for checking and restarting CAN bus
#include "../pecan/pecan.h"       //helper code for CAN stuff
#include "../programConstants.h"  //Constants
// random vitals stuff:
#include "vitalsData.h"
#include "vitalsHB.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

// Initialize space for each task
StaticTask_t sendHB_Buffer;
StackType_t sendHB_Stack[STACK_SIZE];
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE];
StaticTask_t checkStatus_Buffer;
StackType_t checkStatus_Stack[STACK_SIZE];

// send bus status info to telem, also prints it out
void vitals_check_bus_status(void* pvParameters) {
    for (;;) {
        twai_status_info_t status_info;
        esp_err_t err;
        if ((err = twai_get_status_info(&status_info)) == ESP_OK) {
            if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
                printf("Messages to TX: %lu\n", status_info.msgs_to_tx);
                printf("Messages to RX: %lu\n", status_info.msgs_to_rx);
                printf("TX Error Counter: %lu\n", status_info.tx_error_counter);
                printf("RX Error Counter: %lu\n", status_info.rx_error_counter);
                printf("TX Failed Count: %lu\n", status_info.tx_failed_count);
                printf("RX Missed Count: %lu\n", status_info.rx_missed_count);
                printf("RX Overrun Count: %lu\n", status_info.rx_overrun_count);
                printf("Arbitration Lost Count: %lu\n", status_info.arb_lost_count);
                printf("Bus Error Count: %lu\n", status_info.bus_error_count);
                xSemaphoreGive(printfMutex); // Release the mutex.
            } else {
                printf("cant print, in deadlock!\n");
            }

            // send Status update
            int8_t frameNumData = 8;
            int8_t currBit = 0;
            static uint32_t errCnt = 0, txFails = 0, rxOverrun = 0, rxMissed = 0; // records previous value
            uint32_t dataPoints[8] = {(uint32_t) status_info.state,           status_info.tx_error_counter,
                                      status_info.rx_error_counter,           status_info.bus_error_count - errCnt,
                                      status_info.tx_failed_count - txFails,  status_info.rx_overrun_count - rxOverrun,
                                      status_info.rx_missed_count - rxMissed, status_info.msgs_to_rx};
            const int8_t bitLengths[8] = {2, 8, 8, 12, 10, 10, 10, 4};
            const int32_t dataMaxes[8] = {3, 255, 255, 4095, 1023, 1023, 1023, 15};
            errCnt = status_info.bus_error_count; // record current value for next time
            txFails = status_info.tx_failed_count;
            rxOverrun = status_info.rx_overrun_count;
            rxMissed = status_info.rx_missed_count;
            int8_t tempData[8] = {0};
            for (int i = 0; i < frameNumData; i++) { // iterate over each dataPoint that is taken above
                uint32_t unsignedConstrained =
                    formatValue(dataPoints[i], 0, dataMaxes[i]); // constraining. All data have min of 0.
                copyValueToData(&unsignedConstrained, (uint8_t*) tempData, currBit, bitLengths[i]);
                currBit += bitLengths[i];
            }
            // Send the status update
            CANPacket message = {0};
            writeData(&message, tempData, 8);
            message.id = combinedID(busStatusUpdate, vitalsID);
            sendPacket(&message);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// recv Can messages
void recieveMSG() {
    // an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default
    // initialized to default values
    PCANListenParamsCollection plpc = {
        .arr = {{0}},
        .defaultHandler = defaultPacketRecv,
        .size = 0,
    };

    // HB process listen Param
    CANListenParam processBeat;
    processBeat.handler = recieveHeartbeat;
    processBeat.listen_id = combinedID(HBPong, vitalsID); // setting vitals ID doesnt matter, just checking function
    processBeat.mt = MATCH_FUNCTION; // MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits
                                     // of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc, processBeat) != SUCCESS) { // adds the parameter
        mutexPrint("plpc no room");
        while (1);
    }
    //

    // Data process listen Param
    initializeDataTimers(); // initialize timers needed to monitor data
    CANListenParam processData;
    processData.handler = monitorData;
    processData.listen_id =
        combinedID(transmitData, vitalsID); // setting vitals ID doesnt matter, just checking function
    processData.mt = MATCH_FUNCTION; // MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits
                                     // of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc, processData) != SUCCESS) { // adds the parameter
        mutexPrint("plpc no room");
        while (1);
    }
    //

    for (;;) { waitPackets(&plpc); }
}
void app_main(void) {
    base_ESP_init();
    pecanInit config = {.nodeId = vitalsID, .pin1 = defaultPin, .pin2 = defaultPin};
    pecan_CanInit(config);

    TaskHandle_t sendHandler =
        xTaskCreateStaticPinnedToCore( // schedules the task to run the printHello function, assigned to core 0
            sendHB,                    /* Function that implements the task. */
            "HeartBeatSend",           /* Text name for the task. */
            STACK_SIZE,                /* Number of indexes in the xStack array. */
            (void*) 1, /* Parameter passed into the task. */ // should only use constants here. Global variables may be
                                                             // ok? cant be a stack variable.
            3,                                               /* Priority at which the task is created. */
            sendHB_Stack,                                    /* Array to use as the task's stack. */
            &sendHB_Buffer,                                  /* Variable to hold the task's data structure. */
            tskNO_AFFINITY);

    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore( // recieves CAN Messages
        recieveMSG,                                              /* Function that implements the task. */
        "msgRecieve",                                            /* Text name for the task. */
        STACK_SIZE,                                              /* Number of indexes in the xStack array. */
        (void*) 1, /* Parameter passed into the task. */ // should only use constants here. Global variables may be ok?
                                                         // cant be a stack variable.
        tskIDLE_PRIORITY,                                /* Priority at which the task is created. */
        recieveMSG_Stack,                                /* Array to use as the task's stack. */
        &recieveMSG_Buffer,                              /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);

    TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore( // prints out bus status info
        vitals_check_bus_status,                                  /* Function that implements the task. */
        "checkCan",                                               /* Text name for the task. */
        STACK_SIZE,                                               /* Number of indexes in the xStack array. */
        (void*) 1, /* Parameter passed into the task. */ // should only use constants here. Global variables may be ok?
                                                         // cant be a stack variable.
        1,                                               /* Priority at which the task is created. */
        checkStatus_Stack,                               /* Array to use as the task's stack. */
        &checkStatus_Buffer,                             /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);
}
