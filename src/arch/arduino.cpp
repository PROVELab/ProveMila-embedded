#include "Arduino.h"
#include "../common/pecan.hpp"
#include "CAN.h"

int waitPacket(CANPACKET * recv_pack, int listen_id, int (*handler)(CANPACKET *)){
    if (recv_pack == NULL){
        CANPACKET p;
        recv_pack = &p;
        // We can only use this for handler
        // Because stack
    }
    
    int packetsize = CAN.parsePacket();
    int id;
    if (!packetsize &&
        (id = CAN.packetId()) == listen_id
    ){
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        for (int i = 0; i < packetsize; i++){
            recv_pack->data[i] = CAN.read();
        }
        return handler(recv_pack);
    }
    return NOT_RECEIVED;
}


int sendPacket(CANPACKET * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CAN.beginPacket(p->id);
    for (int i = 0; i < p->dataSize; i++){
        CAN.write( p->data[i]);
    }
    if(CAN.endPacket()){
        Serial.println("Success!!!");
    }
    return SUCCESS;

}
