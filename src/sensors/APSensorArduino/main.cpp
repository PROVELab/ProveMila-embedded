#include "../../arduinoSched/arduinoSched.hpp" //used for scheduling
#include "../../pecan/pecan.h"                 //used for CAN
#include "../common/sensorHelper.hpp"          //used for compliance with vitals and sending data
#include "CAN.h"
#include "myDefines.hpp" //contains #define statements specific to this node like myId.
#include <Arduino.h>
#include <avr/wdt.h>

PCANListenParamsCollection plpc = {.arr = {{0}}, .defaultHandler = defaultPacketRecv, .size = 0};
PScheduler ts;
#include <string.h>

int32_t storedAirPressure = 0;

int16_t recvTelemCommand(CANPacket* p) {
    if (p->dataSize != 8) {
        Serial.println("unrecognized telem Command. size must be 8");
        char buffer[20];
        sprintf(buffer, "size: %d", p->dataSize);
        Serial.println(buffer);
    }
    if ((p->data)[0] == customChangeDataFlag) {
        // int32_t newAP = *((int32_t*)((p->data) + 1));
        storedAirPressure = ((uint32_t) p->data[1] << 0) | ((uint32_t) p->data[2] << 8) |
                            ((uint32_t) p->data[3] << 16) | ((uint32_t) p->data[4] << 24);
    }
    return 0;
}

int32_t collect_airPressure() {
    char buffer[64];
    sprintf(buffer, "collecting airPressure: %ld\n", storedAirPressure);
    Serial.println(buffer);
    return storedAirPressure;
}

void setup() {
    Serial.begin(9600);
    Serial.println("sensor begin");
    wdt_enable(WDTO_2S); // enable watchdog with 2s timeout. reset in ts.mainloop

    // declare CanListenparams here, each param has 3 entries.
    // When recv a msg whose id matches 'listen_id' according to matchtype (or 'mt'), 'handler' is called.
    CANListenParam processCMD;
    processCMD.handler = recvTelemCommand;
    processCMD.listen_id = combinedID(TelemetryCommand, telemetryID);
    processCMD.mt = MATCH_EXACT;
    if (addParam(&plpc, processCMD) != SUCCESS) { // adds the parameter
        Serial.println("plpc no room");
        while (1);
    }

    pecanInit config = {.nodeId = myId, .pin1 = defaultPin, .pin2 = defaultPin};
    pecan_CanInit(config);
    sensorInit(&plpc, &ts);
}

void loop() {
    wdt_reset();
    while (waitPackets(&plpc) != NOT_RECEIVED); // handle CAN messages
    ts.execute();                               // Execute scheduled tasks
}
