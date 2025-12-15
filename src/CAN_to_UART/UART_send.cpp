#include "../pecan/pecan.h" //For Can
#include "../programConstants.h"
#include "UART_Com.h"
#include <Arduino.h>
#include <string.h>

// ---------- Framing helpers ----------
#define debugPrintMode \
    0 // Optionally print as ascii hex instead of binary (for reading UART_send outputs on platformio moniter. Telem
      // dashboard is set to read binary, not ASCII Hex)

#if debugPrintMode == 0 // RAW BINARY MODE

static inline void sendByte(uint8_t b) { Serial.write(&b, 1); }

static inline void sendU16_LE(uint16_t v) { Serial.write((uint8_t*) &v, sizeof(v)); }

static inline void sendU32_LE(uint32_t v) { Serial.write((uint8_t*) &v, sizeof(v)); }

static inline void sendU64_LE(uint64_t v) { Serial.write((uint8_t*) &v, sizeof(v)); }

#else // HEX MODE (readable)

static inline void sendByte(uint8_t b) {
    if (b < 0x10) Serial.print('0');
    Serial.print(b, HEX);
    Serial.print(' ');
}

static inline void sendU16_LE(uint16_t v) {
    uint8_t* p = (uint8_t*) &v;
    for (size_t i = 0; i < sizeof(v); i++) {
        if (p[i] < 0x10) Serial.print('0');
        Serial.print(p[i], HEX);
        Serial.print(' ');
    }
}

static inline void sendU32_LE(uint32_t v) {
    uint8_t* p = (uint8_t*) &v;
    for (size_t i = 0; i < sizeof(v); i++) {
        if (p[i] < 0x10) Serial.print('0');
        Serial.print(p[i], HEX);
        Serial.print(' ');
    }
}

static inline void sendU64_LE(uint64_t v) {
    uint8_t* p = (uint8_t*) &v;
    for (size_t i = 0; i < sizeof(v); i++) {
        if (p[i] < 0x10) Serial.print('0');
        Serial.print(p[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

#endif
// -------------------------------------

int16_t CAN_TO_UART(CANPacket* packet) {

    // Build 12-byte payload: 4 byte ID + 8 byte DATA
    uint8_t payload[12] = {0};
    memcpy(payload + 0, &packet->id, 4);
    memcpy(payload + 4, &packet->data, packet->dataSize);                               // copy data to payload
    memset(payload + 4 + packet->dataSize, 0, sizeof(packet->data) - packet->dataSize); // zero unused data bytes

    // Compute 16-bit checksum over payload bytes
    uint16_t csum = in_cksum(payload, sizeof(payload));

    sendByte(0xFF);
    sendByte((uint8_t) (csum & 0xFF)); // low  byte
    sendByte((uint8_t) (csum >> 8));   // high byte
    Serial.write(payload, sizeof(payload));
    return 0;
}

// send ACK after recieving a message from UART
void sendUARTFlag(int nodeID, int8_t flag) {
    CANPacket dataPacket;
    memset(&dataPacket, 0, sizeof(CANPacket));
    dataPacket.id = combinedID(TelemetryCommand, nodeID);
    int8_t response = flag;
    writeData(&dataPacket, (int8_t*) &response, 1);
    CAN_TO_UART(&dataPacket); // sen Ack via UART
}
