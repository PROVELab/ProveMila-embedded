#include "Arduino.h"
#include "../common/pecan.hpp"
// GITHUB NOTES
// We should convert PROVELab GitHub to an organization
// and make Low Voltage leads owners
// Set permissions for the main branch to require pull requests approved by leads + anyone else
// Succession: new leads are added as owners

void setup()
{
    Serial.begin(9600);
}

int receiveHandler(CANPACKET * pack){
    Serial.print("Telemetry received:");
    Serial.println(pack->data);
    return SUCCESS;
}

void loop()
{
    CANPACKET p;
    p.id = 0;
    char hello[] = "Shynn";
    Serial.println("Telemetry Sent.");
    writeData(&p, hello, 5);
    while (waitPacket(&p, 11, receiveHandler) == NOT_RECEIVED){
        ;
    }
}
