#include <stdint.h>

uint16_t in_cksum(const uint8_t* addr, int len) {
    uint32_t sum = 0;
    const uint16_t* w = (const uint16_t*) addr;

    // Sum 16-bit words
    while (len > 1) {
        sum += *w++;
        len -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (len == 1) {
        uint16_t last = 0;
        *((uint8_t*) &last) = *(const uint8_t*) w; // place in low byte
        sum += last;
    }

    // Fold to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t) ~sum;
}
