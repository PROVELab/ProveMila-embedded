#include "../common/pecan.hpp"
#include "mbed.h"

CAN can1(p30, p29, 500E3);

int waitPackets(CANPacket * recv_pack, PCANListenParamsCollection * plpc){
    // If we don't get one passed in,
    // that means the sender doesn't want it
    if (recv_pack == NULL){
        CANPacket p;
        recv_pack = &p;
    }

    CANMessage msg;
    CANListenParam clp;
    int out, id;
    if ((out = can1.read(msg))){
        id = msg.id;
        for (int i = 0; i < plpc->size; i++){
            clp = plpc->arr[i];
            if (matcher[clp.mt](id, clp.listen_id)){
                recv_pack->id = msg.id;
                recv_pack->dataSize = msg.len;
                printf("Msg len: %d\n", msg.len);
                for (int j = 0; j < msg.len; j++){
                    recv_pack->data[j] = msg.data[i];
                }
                return clp.handler(recv_pack);
            }
        }
    }
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