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
}

int receiveHandler(CANPacket * pack){
    Serial.print("Telemetry received:");
    Serial.println(pack->data);
    return SUCCESS;
}

void loop()
{
    CANPacket  p;
    p.dataSize = 8;
    p.id = combinedID(0b1111, 0b0);
    strcpy( p.data, "hi lin!");
    Serial.println("Sending");
    if (sendPacket(&p) == GEN_FAILURE){
        Serial.println(":(");
    }

    delay(1000);
}
