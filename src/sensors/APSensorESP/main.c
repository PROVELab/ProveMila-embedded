#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <string.h>

#include "../../espBase/debug_esp.h"
#include "../../pecan/pecan.h"        //used for CAN
#include "../common/sensorHelper.hpp" //used for compliance with vitals and sending data
#include "myDefines.hpp"              //contains #define statements specific to this node like myId.

#include <stdatomic.h>
// add declerations to allocate space for additional tasks here as needed
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; // buffer that the task will use as its stack

int32_t storedAirPressure = 0;

int16_t recvTelemCommand(CANPacket* p) {
    if (p->dataSize != 8) {
        mutexPrint("unrecognized telem Command. size must be 8");
        char buffer[20];
        sprintf(buffer, "size: %d", p->dataSize);
        mutexPrint(buffer);
    }
    if ((p->data)[0] == customChangeDataFlag) {
        // int32_t newAP = *((int32_t*)((p->data) + 1));
        int32_t newAP = ((uint32_t) p->data[1] << 0) | ((uint32_t) p->data[2] << 8) | ((uint32_t) p->data[3] << 16) |
                        ((uint32_t) p->data[4] << 24);

        atomic_store_explicit(&storedAirPressure, newAP, memory_order_relaxed);
    }
    return 0;
}

int32_t collect_airPressure() {
    int32_t airPressure = atomic_load_explicit(&storedAirPressure, memory_order_relaxed);
    char buffer[64];
    sprintf(buffer, "collecting airPressure: %ld\n", airPressure);
    mutexPrint(buffer);
    return airPressure;
}

void recieveMSG() { // task handles recieving Messages
    PCANListenParamsCollection plpc = {
        .arr = {{0}},
        .defaultHandler = defaultPacketRecv,
        .size = 0,
    };
    sensorInit(&plpc, NULL); // vitals Compliance

    // declare CanListenparams here, each param has 3 entries.
    // When recv a msg whose id matches 'listen_id' according to matchtype (or 'mt'), 'handler' is called.

    CANListenParam processCMD;
    processCMD.handler = recvTelemCommand;
    processCMD.listen_id = combinedID(TelemetryCommand, telemetryID);
    processCMD.mt = MATCH_EXACT;
    if (addParam(&plpc, processCMD) != SUCCESS) { // adds the parameter
        mutexPrint("plpc no room");
        while (1);
    }

    // this task will the call the appropriate ListenParams function when a CAN message is recieved
    for (;;) {
        while (waitPackets(&plpc) != NOT_RECEIVED);
        taskYIELD();
    }
}

void app_main(void) {
    base_ESP_init();
    pecanInit config = {.nodeId = myId, .pin1 = defaultPin, .pin2 = defaultPin};
    pecan_CanInit(config); // initialize CAN

    // Declare tasks here as needed
    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore( // recieves CAN Messages
        recieveMSG,                                              /* Function that implements the task. */
        "msgRecieve",                                            /* Text name for the task. */
        STACK_SIZE,                                              /* Number of indexes in the xStack array. */
        (void*) 1,                                               /* Parameter passed into the task. */
        tskIDLE_PRIORITY,                                        /* Priority at which the task is created. */
        recieveMSG_Stack,                                        /* Array to use as the task's stack. */
        &recieveMSG_Buffer,                                      /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);
}
