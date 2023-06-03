#include "../common/pecan.hpp"
#include "mbed.h"


int waitPacket(CANPacket * recv_pack, int listen_id, int (* handler)(CANPacket *)){
    // If we don't get one passed in,
    // that means the sender doesn't want it
    if (recv_pack == NULL){
        CANPacket p;
        recv_pack = &p;
    }

    CANMessage msg;
    int out;
    if ((out = can1.read(msg))){
        recv_pack->id = msg.id;
        recv_pack->dataSize = msg.len;
        printf("Msg len: %d\n", msg.len);
        for (int i = 0; i < msg.len; i++){
            recv_pack->data[i] = msg.data[i];
        }
        printf("Our function pointer: %s %d\n", recv_pack->data, recv_pack->dataSize);
        return handler(recv_pack);
    }
    // printf("out: %d\n", out);
    return NOT_RECEIVED;
}


int sendPacket(CANPacket * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CANMessage msg(p->id, p->data, p->dataSize);
    if (can1.write(msg)){
        return SUCCESS;
    } return GEN_FAILURE;
}