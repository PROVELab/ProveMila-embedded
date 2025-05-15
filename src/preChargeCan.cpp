#include "../pecan/pecan.h"
#include "driver/gpio.h"
  
// **Figure out CAN packet usage 
void turnOn(CANPacket *message){
    //Switch on the contactors (*** CHANGE TO GPIO FUNCTIONS FOR ESP32 ***)
    digitalWrite(10,HIGH);  // Turn on contacter 1
    digitalWrite(11,HIGH); // Turn on contacter 2
    digitalWrite(12,HIGH); // Turn on contacter 3
}

void shutoff(CANPacket *message){
    digitalWrite(10,LOW);  // Turn off contacter 1
    digitalWrite(11,LOW); // Turn off contacter 2
    digitalWrite(12,LOW); // Turn off contacter 3   
}

int main() {
    CANListenParam canParams[2];

    //set up the CAN packet to listen for turn on/off messages
    CANListenParam turnOnParam = {0}; 
    turnOnParam.listen_id = 0x100; //example id, change to actual id
    turnOnParam.handler = turnOn; //function to call when this id is received
    turnOnParam.mt = MATCH_EXACT; //exact match

    CANListenParam shutoffParam = {0}; 
    shutoffParam.listen_id = 0x200; //example id, change to actual id
    shutoffParam.handler = shutoff; //function to call when this id is received
    shutoffParam.mt = MATCH_EXACT; //exact match

    canParams[0] = turnOnParam;
    canParams[1] = shutoffParam;

    //configure CAN bus
    CANPacket preChargeCanPacket = {0};
    PCANListenParamsCollection preChargeCanParams = {0};
    preChargeCanParams.arr = canParams; 
    preChargeCanParams.size = 2; //number of params in the collection

    
    while(1){
        //wait for a packet to be received
        int ret = waitPackets(&preChargeCanPacket, preChargeCanParams);
        if(ret < 0){
            serial.println("Error receiving packet");
            continue;
        }
    }
}


