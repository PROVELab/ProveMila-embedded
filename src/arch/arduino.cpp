#include "../common/pecan.hpp"
#include "Arduino.h"
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

int waitPackets(CANPACKET * recv_pack, CANLISTEN_PARAM * params){

} // Pass by Ref

int sendPacket(CANPACKET * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CAN.beginPacket(p->id);
    CAN.write(p->data, p->dataSize);
    if (!CAN.endPacket()){
        return SUCCESS;
    } return GEN_FAILURE;
}
