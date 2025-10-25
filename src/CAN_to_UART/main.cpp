#include "../arduinoSched/arduinoSched.hpp" //used for scheduling
#include "../pecan/pecan.h"                 //For Can
#include "../programConstants.h"
#include "Arduino.h"
#include "CAN.h"
#include <avr/wdt.h>

#include "UART_Com.h"
#include <stdint.h>

// Sets the CAN_TO_UART function as the defaultHandler, ie, forwards all messages via UART to homebase (my computer)
PCANListenParamsCollection plpc = {.arr = {{0}}, .defaultHandler = CAN_TO_UART, .size = 0};
// PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};

PScheduler ts;

// Forward message from UART (ie, homebase) to CAN
void UART_TO_CAN() {
    uint8_t* bytes;
    if (recvFrame(bytes)) {
        CANPacket msgForward;
        memset(&msgForward, 0, sizeof(msgForward));

        int8_t flag = customChangeDataFlag;
        writeData(&msgForward, &flag, 1);
        writeData(&msgForward, (int8_t*) bytes, 7);
        msgForward.id = combinedID(TelemetryCommand, telemetryID);
        sendPacket(&msgForward);
        sendUARTFlag(telemetryID, telemetryCommandAck); // send Ack via UART
    }
}

void setup() {
    Serial.begin(115200);
    wdt_reset();
    while (!Serial);
    Serial.println("CAN_TO_UART_START");
    wdt_enable(WDTO_2S); // enable watchdog with 2s timeout. reset in ts.mainloop
    pecanInit config = {.nodeId = telemetryID, .pin1 = defaultPin, .pin2 = defaultPin};
    pecan_CanInit(config);
    vitalsInit(&plpc, telemetryID);

    ts.scheduleTask(UART_TO_CAN, 1000);
}

// new Arduino base loop
void loop() {
    wdt_reset();
    ts.execute();
    while (waitPackets(&plpc) != NOT_RECEIVED);
}
