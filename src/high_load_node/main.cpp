#include <Arduino.h>
#include <CAN.h>

void setup () {
    Serial.begin(9600);
    while (!Serial);

    Serial.println("CAN Sender");

    // start the CAN bus at 500 kbps
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
}

void loop () {
    uint8_t buffer[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    CAN.beginPacket(0x12);
    CAN.write(buffer, 8);
    CAN.endPacket();
}