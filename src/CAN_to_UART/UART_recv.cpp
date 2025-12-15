#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "UART_Com.h"

#define SOF 0xFF // Start Of Frame byte

// recv a frame from HQ
bool recvFrame(uint8_t*& out) {
    static uint8_t frame[10] = {0}; // [CHK16][DATA8]
    static int8_t have = -1;

    while (Serial.available() > 0) {
        uint8_t b = (uint8_t) Serial.read();

        if (have == -1) {
            if (b == SOF) { have = 0; }
            continue;
        }

        frame[have++] = b;

        if (have != sizeof(frame)) {
            continue; // not enough bytes yet
        }
        // We have full message

        // Extract checksum and payload
        uint16_t chk = ((uint16_t) frame[1] << 8) | frame[0];
        const uint8_t* payload = &frame[2]; // 8 bytes

        uint16_t calc = in_cksum(payload, 8);

        if (chk == calc) {
            out = &frame[2]; // set out to where data starts
            have = -1;       // ready for next frame
            return true;     // valid message
        }

        sendUARTFlag(telemetryID, telemetryCommandCRCError);

        // Bad checksum: resync by scanning inside the 10 bytes after SOF for next SOF
        int nextSOF = -1;
        for (uint8_t i = 0; i < sizeof(frame); ++i) {
            if (frame[i] == SOF) {
                nextSOF = i;
                break;
            }
        }
        if (nextSOF >= 0) {
            uint8_t keep = sizeof(frame) - (nextSOF + 1); // bytes to keep starting at that SOF
            memmove(frame, frame + nextSOF + 1, keep);    // move starting after SOF to start of buffer
            have = keep;                                  // we now have [SOF][...] in buffer; continue reading
        } else {
            have = -1; // drop and wait for a fresh SOF
        }
    }

    return false; // no full frame yet
}
