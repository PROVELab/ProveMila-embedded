#include "CAN.h"
#include "pecan.h"

#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void flexiblePrint(const char* str) { Serial.println(str); }
#define defaultCSPin  10
#define defaultIntPin 2

// Note: the MCP 2515 (with the shield we use) can buffer only 2 CAN messages. The Sandeepmistry Can library we use uses
// this feature
//  Since we may have some actions that take a while (roughly anything > 1ms), its important to have an interrupt to
//  store messages in a SW queue imediately. (For example, responses to HB wil come quickly like this.)
void onReceive(int packetSize);

// default pins work with elecFreaks shield as is. non-defaultPins will required different/modified HW setup.
void pecan_CanInit(pecanInit config) {
    const int irqPin = (config.pin1 == defaultPin) ? defaultIntPin : config.pin1;
    const int chipSelectPin = (config.pin2 == defaultPin) ? defaultCSPin : config.pin2;

    // Use non-default pins only if needed
    if (irqPin != defaultIntPin || chipSelectPin != defaultCSPin) {
        // CAN.setPins(chipSelectPin, irqPin);
    }
    CAN.onReceive(onReceive);

    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    sendStatusUpdate(initFlag, config.nodeId);
    return;
}

// Queue for recieving messages:
#define PACKET_QUEUE_SIZE 16
volatile bool queueOverrunFlag = false;

CANPacket packetQueue[PACKET_QUEUE_SIZE];
volatile uint8_t queueHead = 0; // ISR writes
volatile uint8_t queueTail = 0; // main writes
volatile bool flagGotCalled = false;

static inline uint8_t incrementIndex(uint8_t i) {
    uint8_t n = (uint8_t) (i + 1);
    return (n >= PACKET_QUEUE_SIZE) ? 0 : n;
}

static inline bool queueIsEmpty(void) { return queueHead == queueTail; }

static inline void queuePacket(const CANPacket& pkt) {
    uint8_t nextHead = incrementIndex(queueHead);
    if (nextHead == queueTail) { // indicate we dropped a packet
        queueOverrunFlag = true;
        return;
    } else { // add packet to queue
        packetQueue[queueHead] = pkt;
        queueHead = nextHead;
    }
}

static inline bool queuePacketPop(CANPacket* outPkt) {
    bool ok = false;
    noInterrupts(); // must be atomic w.r.t. ISR
    if (!queueIsEmpty()) {
        *outPkt = packetQueue[queueTail];
        queueTail = incrementIndex(queueTail);
        ok = true;
    }
    interrupts();
    return ok;
}
//  //

bool (*matcher[3])(uint32_t, uint32_t) = {
    exact, matchID, matchFunction}; // could alwys be moved back to pecan.h as an extern variable if its needed
                                    // elsewhere? I am not sure why this was declared there in the first place

// expects waitPackets to 0 initialize irellevant id and data.
// Want to keep ISR as short as possible
void onReceive(int packetSize) {
    static CANPacket recv_pack;
    recv_pack.id = CAN.packetId();
    recv_pack.extendedID = CAN.packetExtended();
    recv_pack.dataSize = packetSize;
    recv_pack.rtr = CAN.packetRtr();

    // Read packet data
    for (int8_t index = 0; index < recv_pack.dataSize; index++) { recv_pack.data[index] = CAN.read(); }
    queuePacket(recv_pack); // queue packet for later use
}

// Matches any recieved packets with their handler
// Not thread-safe (only call from one thread). The packet reference is overriden upon call.
// returns value of the matching function, or NOT_RECIEVED for no new messages
int16_t waitPackets(PCANListenParamsCollection* plpc) {
    static CANPacket recv_pack = {0};
    // TODO: would be nice to check for RX overruns. However, the CAN libarary we use doesnt expose this
    // Realistically it isn't necessary, but its worth noting that its possible to drop packets if we have
    // other interrupts taking a long time, preventing onReceieve from getting called
    // if(queueOverrunFlag){
    //     sendStatusUpdate( myId)
    // }

    if (queuePacketPop(&recv_pack)) {
        // limit id. Theoretically shouldnt be necessary, but better to be safe
        if (recv_pack.extendedID) {
            recv_pack.id &= 0x1FFFFFFF; // Limit to first 29 bits
        } else {
            recv_pack.id &= 0x7FF; // Limit to first 11 bits
        }
        if (recv_pack.rtr == true) {
            recv_pack.dataSize = 0;
        } else {
            recv_pack.dataSize = constrain(recv_pack.dataSize, 0, MAX_SIZE_PACKET_DATA); // to be safe
        }
        // set garbage data to 0.
        memset(recv_pack.data + recv_pack.dataSize, 0, MAX_SIZE_PACKET_DATA - recv_pack.dataSize);

        CANListenParam clp;
        for (int16_t i = 0; i < plpc->size; i++) {
            clp = plpc->arr[i];
            if (matcher[clp.mt](recv_pack.id, clp.listen_id)) { return clp.handler(&recv_pack); }
        }
        plpc->defaultHandler(&recv_pack);
        return 0; // Success
    }
    return NOT_RECEIVED;
}

void sendPacket(CANPacket* p) { // note: if your id is longer than 11 bits it made into
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        Serial.println("Packet Too Big");
        return;
    }
    bool success;
    do {
        if (p->rtr) { p->dataSize = 1; }
        if (p->id > 0b11111111111) {
            CAN.beginExtendedPacket(p->id, p->dataSize, p->rtr);
        } else {
            CAN.beginPacket(p->id, p->dataSize, p->rtr);
        }

        if (p->rtr == false) {
            for (int8_t i = 0; i < p->dataSize; i++) { CAN.write(p->data[i]); }
        }

        success = CAN.endPacket();
        if (!success) {
            Serial.println("message send failure\n");
            delay(5); // small delay before trying again. Dont want to blow up Bus.
        }
    } while (!success); // Attempt to send the packet on a loop
    // Expects watchdog timer to reset us if we get bricked trying to send a message
}
