
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pecan/pecan.h"

static uint16_t savedId;

// function to respond to HB. Thats really all this file does.
static int16_t respondToHB(CANPacket* recvPack) {

    (void) recvPack; // if unused in your implementation
    char buffer[30];
    sprintf(buffer, "saved id: %u", savedId);
    flexiblePrint(buffer);
    flexiblePrint("responding to HB\n");
    CANPacket responsePacket;
    memset(&responsePacket, 0, sizeof(CANPacket));

    responsePacket.id = combinedID(HBPong, savedId); // send Pong from myId
    setRTR(&responsePacket);
    sendPacket(&responsePacket);

    return 1;
}

void vitalsInit(PCANListenParamsCollection* plpc, uint16_t nodeID) {
    flexiblePrint("init vitals");
    CANListenParam babyDuck = {.listen_id = combinedID(HBPing, vitalsID), .handler = respondToHB, .mt = MATCH_EXACT};
    savedId = nodeID;

    if (addParam(plpc, babyDuck) != SUCCESS) {
        flexiblePrint("plpc no room");
        while (1) { /* trap */
        }
    }
}
