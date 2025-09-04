#include "Arduino.h"

#include "pecan.h"
#include "CAN.h"

#include <Arduino.h>

void pecan_CanInit(pecanInit config){
    const int defaultTxPin = 5;
    const int defaultRxPin = 4;
    if(config.txPin != defaultPin || config.rxPin !=defaultPin){    //using non-default pins has not been tested on arduino, but option is available
        config.txPin= config.txPin == defaultPin ? defaultTxPin : config.txPin;
        config.rxPin= config.rxPin == defaultPin ? defaultRxPin : config.rxPin;
        CAN.setPins(config.txPin, config.rxPin);
    }
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    int16_t err;
    if((err= sendStatusUpdate(initFlag, config.nodeId))){
        char buffer[50];
        sprintf(buffer, "error sending INIT Flag: %d\n", err);  // Convert the int8_t to a string
        printf(buffer);  
    }
    return;
}

int16_t defaultPacketRecv(CANPacket *packet) {  //this is only to be used by vitals for current
    Serial.print("Default Func: id ");
    Serial.print(packet->id);
    Serial.print(",  data ");
    for(int i=0;i<packet->dataSize;i++){
        Serial.print(*(packet->data));
        Serial.print(" ");
    }
    Serial.println(" ");
    return 0;
}
bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};   //could alwys be moved back to pecan.h as an extern variable if its needed elsewhere? I am not sure why this was declared there in the first place

int16_t waitPackets(CANPacket *recv_pack, PCANListenParamsCollection *plpc) {   
    if (recv_pack == NULL) {    //final product code cant be calling this with NULL, since packet loses scope
        CANPacket p;
        recv_pack = &p;
    }

    int8_t packetsize;
    CANListenParam clp;
    if ((packetsize = CAN.parsePacket())) {
        Serial.println("recieving Packet: ");
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        recv_pack->rtr= CAN.packetRtr();
        memset(recv_pack->data, 0, 8);  //re-initialize data to all 0.
        // Read and temporarily store all the packet data into PCAN
        // packet (on stack memory)
        for (int8_t j = 0; j < recv_pack->dataSize; j++) {
            recv_pack->data[j] = CAN.read();
        }
        // Then match the packet id with our params; if none
        // match, use default handler
        Serial.println("trying match");
        for (int16_t i = 0; i < plpc->size; i++) {
            clp = plpc->arr[i];
            if (matcher[clp.mt](recv_pack->id, clp.listen_id)) {
                return clp.handler(recv_pack);
            }
        }
        return plpc->defaultHandler(recv_pack);
    }
    return NOT_RECEIVED;
}

void sendPacket(CANPacket *p) {  //note: if your id is longer than 11 bits it made into
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        Serial.println("Packet Too Big");
        return;
    }
    do {
        if(p->id>0b11111111111){
        CAN.beginExtendedPacket(p->id, p->dataSize, p->rtr);
        }else{
            CAN.beginPacket(p->id, p->dataSize, p->rtr);
        }
        for (int8_t i = 0; i < p->dataSize; i++) {
            CAN.write(p->data[i]);
        }
        bool success=CAN.endPacket();
        if(!success){
            delay(5);   //small delay before
        }
    } while(!success);  //Attempt to send the packet on a loop
    //Expects watchdog timer to reset us if we get bricked trying to send a message
}