#include "../programConstants.h"
#include "pecan.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  //for snprintf
#include <string.h> // memcpy

uint32_t combinedID(uint32_t fn_id, uint32_t node_id) { return (fn_id << 7) + node_id; }
uint32_t combinedIDExtended(uint32_t fn_id, uint32_t node_id, uint32_t extension) {
    return combinedID(fn_id, node_id) + (extension << 11);
}

void setSensorID(CANPacket* p, uint8_t sensorId) { p->data[0] = sensorId; }

int16_t defaultPacketRecv(CANPacket* p) {
    char buf[48];
    // Print the header
    snprintf(buf, sizeof(buf), "Default recv: id %ld\n with data:", p->id);
    flexiblePrint(buf);

    // Print each data element
    if (p->rtr == false) {
        for (int i = 0; i < p->dataSize; i++) {
            snprintf(buf, sizeof buf, " %02X", (unsigned) p->data[i]); // two-digit uppercase hex
            flexiblePrint(buf);
        }
    }

    // Print the final newline
    flexiblePrint("\n");
    return 0;
}

int16_t addParam(PCANListenParamsCollection* plpc, CANListenParam clp) {
    if (plpc->size + 1 > MAX_PCAN_PARAMS) {
        return NOSPACE;
    } else {
        plpc->arr[plpc->size] = clp;
        plpc->size++;
        return SUCCESS;
    }
}
int16_t setRTR(CANPacket* p) { // makes the given packet an RTR packet
    if (p->dataSize != 0) {
        return 1; // this packet has data written to it, it cant also be an rtr packet
    }
    p->rtr = 1;
    return 0;
}
int16_t setExtended(CANPacket* p) { // makes the given packet an extended ID packet (so can send 29 bits of id, instead
                                    // of just first 11)
    p->extendedID = 1;
    return 0;
}

int16_t writeData(CANPacket* p, int8_t* dataPoint, int16_t size) {
    if (p->rtr) {
        return -4; // this is an rtr packet, can not write data
    }
    const size_t originalSize = p->dataSize;
    if (originalSize + size > MAX_SIZE_PACKET_DATA) { return NOSPACE; }
    memcpy(&(p->data[originalSize]), dataPoint, size);
    (p->dataSize) += size;
    return SUCCESS;
}

// returns value constrained to min of min, and max of max
int32_t squeeze(int32_t value, int32_t min, int32_t max) { return (value < min) ? min : (value > max ? max : value); }

bool exact(uint32_t id, uint32_t mask) { // does not check extended bits of Id
    return (id & 0b11111111111) == mask;
}
uint32_t formatValue(int32_t value, int32_t min, int32_t max) { return (uint32_t) (squeeze(value, min, max) - min); }
int16_t
copyValueToData(uint32_t* value, uint8_t* target, int8_t startBit,
                int8_t numBits) { // copies the first numBits of value into target starting at startBit of target
    if (numBits <= 0 || startBit < 0 || startBit + numBits > 64) return 1; // function called in invalid circumstance

    // Treat target and source as 64-bit integers
    uint64_t* target64 = (uint64_t*) target;
    uint64_t source = (uint64_t) (*value) & ((1ULL << numBits) - 1); // Mask the relevant bits from value

    *target64 |= (source << startBit); // Shift the masked value to the correct position and set it in target
    return 0;
}

// inverse of the above function
int16_t
copyDataToValue(uint32_t* target, uint8_t* data, int8_t startBit,
                int8_t numBits) { // relies on mc using little endian (since interpretting array of bytes as int)
    if (numBits <= 0 || startBit < 0 || startBit + numBits > 64) return 1;

    // Treat target as a 64-bit integer (for up to 8 bytes)
    uint64_t* target64 = (uint64_t*) data;                   // Cast target to uint64_t pointer
    uint64_t mask = ((1ULL << numBits) - 1) << startBit;     // Create a mask for the desired bit range
    uint64_t extractedBits = (*target64 & mask) >> startBit; // Extract and shift the relevant bits

    *target = (int32_t) extractedBits; // Update the value with the extracted bits
    return 0;
}

void sendStatusUpdate(uint8_t flag, uint32_t Id) {
    CANPacket statusUpdatePacket;
    memset(&statusUpdatePacket, 0, sizeof(CANPacket));
    statusUpdatePacket.id = combinedID(statusUpdate, Id);
    writeData(&statusUpdatePacket, (int8_t*) &flag, 1);
    sendPacket(&statusUpdatePacket);
}

// note: ID sent over CAN is 11 bit long, with first 7 bitsbeing the identifier of sending node, and last 4 bits being
// the function code
bool matchID(uint32_t id, uint32_t mask) { // check if the 7 bits of node ID must match mask
    return getNodeId(id) == getNodeId(mask);
}

bool matchFunction(uint32_t id, uint32_t mask) {     // mask should contain 4 bit functoin code in bits 7-10.
    return getFunctionId(id) == getFunctionId(mask); // only compares the functionCodes
}
