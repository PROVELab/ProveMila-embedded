#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

#include "../../espBase/debug_esp.h"
#include "../../pecan/pecan.h"                   //used for CAN
#include "../../sensors/common/sensorHelper.hpp" //used for compliance with vitals and sending data
#include "myDefines.hpp"                         //contains #define statements specific to this node like myId.

#include "../../mcu/motor_h300/h300.h"
#include "../../mcu/vsr.h"
#include "../powerSensor/powerSensor.h"
#include "pedalInterpolation.h"
#include "pedal_sensor.h"

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

#include <stdint.h>

// Tune these:
#define PEDAL_START 10
#define PEDAL_END   100
#define MIN_SPEED   0
#define MAX_SPEED   MPH_TO_RPM(20) // max 10 mph

static inline int scale_pedal_to_speed(int a) {
    if (a < PEDAL_START) return 0;        // deadband -> 0
    if (a >= PEDAL_END) return MAX_SPEED; // clamp high

    const int in_span = (PEDAL_END - PEDAL_START);
    const int out_span = (MAX_SPEED - MIN_SPEED);

    // Linear map with rounding to nearest, using wide intermediate to avoid overflow.
    int64_t num = (int64_t) (a - PEDAL_START) * out_span + in_span / 2;
    return MIN_SPEED + (int) (num / in_span);
}

int32_t collect_pedalPowerReadingmV() {
    // mutexPrint("collecting pedalPowerReadingmV\n");
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

    // speed stuff
    int a = (ADC_Readings[reading1_Index] < ADC_Readings[reading2_Index]) ? ADC_Readings[reading1_Index]
                                                                          : ADC_Readings[reading2_Index];
    // int a = ADC_Readings[reading1_Index];

    int b = 0;
    if (a < 10) {
        // Current 0
        b = 0;
    } else if (a > 100) {
        b = 600;
    } else {
        b = 6 * a;
    }

    // Send the speed (if necessary)
    vehicle_status_reg_s* vsr = &vehicle_status_register; // easier to type
    bool use_pedal = false;
    ACQ_REL_VSRSEM_W(pedal, {
        vsr->pedal.pedal_position_pct = (float) a;
        vsr->pedal.pedal_raw_1 = (float) ADC_Readings[reading1_Index];
        vsr->pedal.pedal_raw_2 = (float) ADC_Readings[reading2_Index];
        vsr->pedal.pedal_supply_voltage = (float) ADC_Readings[pedalPower_Index];

        vsr->pedal.tx_value = b;
        use_pedal = vsr->pedal.use_pedal;
    });

    if (use_pedal) {
        ACQ_REL_VSRSEM_W(motor_control, { vsr->motor_control.current_reference = (int32_t) b; });
    }

    return ADC_Readings[reading2_Index];
}

int16_t defaultPacketRecv2(CANPacket* p) { return 1; }

void recieveMSG(void* param) { // task handles recieving Messages
    PCANListenParamsCollection* plpc = (PCANListenParamsCollection*) param;
    sensorInit(plpc, NULL); // vitals Compliance

    // declare CanListenparams here, each param has 3 entries:
    // When recv msg with id = 'listen_id' according to matchtype (or 'mt'), 'handler' is called.

    // task calls the appropriate ListenParams function when a CAN message is recieved
    for (;;) {
        while (waitPackets(plpc) != NOT_RECEIVED);
        taskYIELD();
    }
}

void pedal_main(PCANListenParamsCollection* plpc) {
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
    TaskHandle_t _recieveHandler = xTaskCreateStaticPinnedToCore( // recieves CAN Messages
        recieveMSG,                                               /* Function that implements the task. */
        "msgRecieve",                                             /* Text name for the task. */
        STACK_SIZE,                                               /* Number of indexes in the xStack array. */
        (void*) plpc,       /* Task Parameter. Must remain in scope or be constant!*/
        10,                 /* Priority at which the task is created. */
        recieveMSG_Stack,   /* Array to use as the task's stack. */
        &recieveMSG_Buffer, /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);    // assign to either core
}
