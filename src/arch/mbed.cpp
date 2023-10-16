#include "../common/pecan.hpp"
#include "mbed.h"
#include "mbed_events.h"

CAN can1(p30, p29, 500E3);

// Specific Packet stuff
int16_t waitPacket(CANPacket * recv_pack, int16_t listen_id, int16_t (*handler)(CANPacket *)){
    // If we don't get one passed in,
    // that means the sender doesn't want it
    if (recv_pack == NULL){
        CANPacket p;
        recv_pack = &p;
    }

    CANMessage msg;
    int8_t out;
    if ((out = can1.read(msg))){
        recv_pack->id = msg.id;
        recv_pack->dataSize = msg.len;
        printf("Msg len: %d\n", msg.len);
        for (int8_t i = 0; i < msg.len; i++){
            recv_pack->data[i] = msg.data[i];
        }
        return handler(recv_pack);
    }
    return NOT_RECEIVED;
}

int16_t sendPacket(CANPacket * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CANMessage msg(p->id, (uint8_t*) p->data, p->dataSize);
    if (can1.write(msg)){
        return SUCCESS;
    } return GEN_FAILURE;
}

Scheduler::Scheduler(){
}

PCAN_ERR Scheduler::scheduleTask(Task t){
    if (ctr >= MAX_TASK_COUNT){
        return NOSPACE;
    }
    this->tasks[ctr] = t;
    ctr++;
    printf("Counter incremented. Currently %d\n", ctr);
    return SUCCESS;
}

void Scheduler::mainloop(int8_t * inp){
    EventQueue queue;

    Thread tOutput, tOutput2;
    printf("Mainloop\n");
    for (uint16_t i = 0;i < ctr; i++){
        Task t = this->tasks[i];
        ((UserAllocatedEvent<void (*)(uint16_t), void (uint16_t)> *)inp)[i] = make_user_allocated_event(t.function, i);
        ((UserAllocatedEvent<void (*)(uint16_t), void (uint16_t)> *)inp)[i].delay(t.delay);
        ((UserAllocatedEvent<void (*)(uint16_t), void (uint16_t)> *)inp)[i].period(t.interval);
        ((UserAllocatedEvent<void (*)(uint16_t), void (uint16_t)> *)inp)[i].call_on(&queue);

    }
    printf("Dispatching\n");
    tOutput.start(callback(&queue, &EventQueue::dispatch_forever));
    tOutput2.start(callback(&queue, &EventQueue::dispatch_forever));
    while(1);
}