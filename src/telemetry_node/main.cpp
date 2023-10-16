#include "Arduino.h"
#include "CAN.h"
#include "../common/pecan.hpp"
// GITHUB NOTES
// We should convert PROVELab GitHub to an organization
// and make Low Voltage leads owners
// Set permissions for the main branch to require pull requests approved by leads + anyone else
// Succession: new leads are added as owners

void setup()
{
    Serial.begin(9600);
    // start the CAN bus at 500 kbps
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN, joystick failed!");
        while (1);
    }
    Serial.println("Telemetry startup succesful");
    // CAN.loopback();

}

int receiveHandler(CANPacket * pack){
    // Serial.print("Telemetry received:");
    // Serial.println(pack->data);
    // return SUCCESS;
}

void loop()
{
    // CANPacket  p;
    // char hello[] = "Shynn\0";
    // p.dataSize = 0;
    // p.id = 11;

    // Serial.println("Telemetry Sent.");
    // writeData(&p, hello, 6);
    // Serial.println(p.data);
    // sendPacket(&p);
    // Serial.println("Post Send;");
    // // Serial.println(CAN.parsePacket());
    // delay(1000);
}
