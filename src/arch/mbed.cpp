#include "../common/pecan.hpp"
#include "mbed.h"

CAN can1(p30, p29);

int waitPacket(CANPACKET * recv_pack, int listen_id, int (*handler)(CANPACKET *)){
    // If we don't get one passed in,
    // that means the sender doesn't want it
    if (recv_pack == NULL){
        CANPACKET p;
        recv_pack = &p;
    }

    CANMessage msg;
    if (can1.read(msg) &&
        (msg.id == listen_id)
    ){
        recv_pack->id = msg.id;
        recv_pack->dataSize = msg.len;
        for (int i = 0; i < msg.len; i++){
            recv_pack->data[i] = msg.data[i];
        }
        return handler(recv_pack);
    }
    return NOT_RECEIVED;
}


int sendPacket(CANPACKET * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CANMessage msg(p->id, p->data, p->dataSize);
    if (!can1.write(msg)){
        return SUCCESS;
    } return GEN_FAILURE;
}