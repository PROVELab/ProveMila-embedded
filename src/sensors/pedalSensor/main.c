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

#include "../powerSensor/powerSensor.h"
#include "pedalInterpolation.h"

// add declerations to allocate space for additional tasks here as needed
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; // buffer that the task will use as its stack

#define numADCChannels 3

typedef enum {
    pedalPower_Index = 0,
    reading1_Index = 1,
    reading2_Index = 2,
} ADC_Indices;

int32_t ADC_Readings[numADCChannels];
selfPowerStatus_t ADC_ReadingStatuses[numADCChannels];

int32_t collect_pedalPowerReadingmV() {
    mutexPrint("collecting pedalPowerReadingmV\n");
    collectSelfPowerAllmV(ADC_Readings, ADC_ReadingStatuses);

    // 4) Report/guard after read
    selfPowerStatusCheck(ADC_ReadingStatuses, numADCChannels, myId);

    if (ADC_ReadingStatuses[pedalPower_Index] != READ_SUCCESS || ADC_Readings[pedalPower_Index] < 4500 ||
        ADC_Readings[pedalPower_Index] > 7500) {
        // Indicate status failure on pedal readings. they can't be used
        // with a power reading this terrible.
        ADC_ReadingStatuses[reading1_Index] = READ_CRITICAL;
        ADC_ReadingStatuses[reading2_Index] = READ_CRITICAL;
    }
    char buffer[64];
    sprintf(buffer, "collecting pedalPowermV: %ld\n", ADC_Readings[pedalPower_Index]);
    mutexPrint(buffer);
    return ADC_Readings[pedalPower_Index];
}

int32_t collect_pedalReadingOne() {
    if (ADC_ReadingStatuses[reading1_Index] != READ_SUCCESS) {
        // we should not be driving if we cant read something
        ADC_Readings[reading1_Index] = -1;
        mutexPrint("invalid pedal reading 1\n");
    } else {
        // transform pedal power reading mV to pedal reading percentage
        ADC_Readings[reading1_Index] =
            transformPedalReading(ADC_Readings[reading1_Index], ADC_Readings[pedalPower_Index], risingPedalIndex);
    }
    char buffer[64];
    sprintf(buffer, "collecting pedalReadingOne: %ld\n", ADC_Readings[reading1_Index]);
    mutexPrint(buffer);
    return ADC_Readings[reading1_Index];
}

int32_t collect_pedalReadingTwo() {
    if (ADC_ReadingStatuses[reading2_Index] != READ_SUCCESS) {
        // we should not be driving if we cant read something
        ADC_Readings[reading2_Index] = -1;
        mutexPrint("invalid pedal reading 2\n");
    } else {
        // transform pedal power reading mV to pedal reading percentage
        ADC_Readings[reading2_Index] =
            transformPedalReading(ADC_Readings[reading2_Index], ADC_Readings[pedalPower_Index], fallingPedalIndex);
    }
    char buffer[64];
    sprintf(buffer, "collecting pedalReadingTwo: %ld\n", ADC_Readings[reading2_Index]);
    mutexPrint(buffer);
    return ADC_Readings[reading2_Index];
}

int16_t defaultPacketRecv2(CANPacket* p) { return 1; }

void recieveMSG() { // task handles recieving Messages
    PCANListenParamsCollection plpc = {
        .arr = {{0}},
        .defaultHandler = defaultPacketRecv2,
        .size = 0,
    };
    sensorInit(&plpc, NULL); // vitals Compliance

    // declare CanListenparams here, each param has 3 entries:
    // When recv msg with id = 'listen_id' according to matchtype (or 'mt'), 'handler' is called.

    // task calls the appropriate ListenParams function when a CAN message is recieved
    for (;;) {
        while (waitPackets(&plpc) != NOT_RECEIVED);
        taskYIELD();
    }
}

void app_main(void) {
    base_ESP_init();
    pecanInit config = {.nodeId = myId, .pin1 = defaultPin, .pin2 = defaultPin};
    pecan_CanInit(config); // initialize CAN

    // 1) Build your per-channel configs (order matters)
    selfPowerConfig channels[numADCChannels] = {
        {.ADCPin = VP_Pin, .R1 = 114000, .R2 = 57000},  //
        {.ADCPin = VN_Pin, .R1 = 300000, .R2 = 150000}, // rising (white wire)
        {.ADCPin = 35, .R1 = 303000, .R2 = 198000},     // falling (red wire)
    };

    // 2) Initialize; get per-channel init statuses
    const int ADCUnit = 1; // continous is only valid for ADC Unit 1 (not 2)
    selfPowerStatus_t init_status[numADCChannels];
    initializeSelfPower(channels, numADCChannels, ADCUnit, init_status);

    // Optionally report/init-guard
    selfPowerStatusCheck(init_status, numADCChannels, myId);

    // Declare tasks here as needed
    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore( // recieves CAN Messages
        recieveMSG,                                              /* Function that implements the task. */
        "msgRecieve",                                            /* Text name for the task. */
        STACK_SIZE,                                              /* Number of indexes in the xStack array. */
        (void*) 1,          /* Task Parameter. Must remain in scope or be constant!*/
        tskIDLE_PRIORITY,   /* Priority at which the task is created. */
        recieveMSG_Stack,   /* Array to use as the task's stack. */
        &recieveMSG_Buffer, /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);    // assign to either core
}
