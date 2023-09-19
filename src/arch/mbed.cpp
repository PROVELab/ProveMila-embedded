#include "../common/pecan.hpp"
#include "mbed.h"
#include "mbed_events.h"


CAN can1(p30, p29, 500E3);

// Specific Packet stuff
int waitPacket(CANPacket * recv_pack, int listen_id, int (*handler)(CANPacket *)){
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
        return handler(recv_pack);
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

void Scheduler::mainloop(){
    EventQueue queue(32* EVENTS_EVENT_SIZE);

    Thread tOutput;
    for (int i = 0;i < ctr; i++){
        Task t = tasks[i];
        queue.call_every(t.interval, t.function);
    }

    tOutput.start(callback(&queue, &EventQueue::dispatch_forever));
    tOutput.join();
}