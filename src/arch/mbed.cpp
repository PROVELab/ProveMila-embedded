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

void func(int a){}

Scheduler::Scheduler() : queue(0){
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

void Scheduler::mainloop(char * inp){

    Thread tOutput, tOutput2;
    printf("Mainloop\n");
    // ((UserAllocatedEvent<void (*)(int), void (int)> *)inp)[0] = make_user_allocated_event(this->tasks[0].function, 0);
    // auto va = make_user_allocated_event(this->tasks[0].function, 0);
    
    // va.delay(tasks[0].delay);
    // va.period(tasks[0].interval);
    // va.call_on(&this->queue);
    // events::UserAllocatedEvent<void (*)(int), void (int)> va;
    for (int i = 0;i < ctr; i++){
        printf("I, ctr %d %d\n", i, ctr);
        Task t = this->tasks[i];
        ((UserAllocatedEvent<void (*)(int), void (int)> *)inp)[i] = make_user_allocated_event(t.function, i);
        ((UserAllocatedEvent<void (*)(int), void (int)> *)inp)[i].delay(t.delay);
        ((UserAllocatedEvent<void (*)(int), void (int)> *)inp)[i].period(t.interval);
        ((UserAllocatedEvent<void (*)(int), void (int)> *)inp)[i].call_on(&queue);
        printf("I, ctr %d %d\n", i, ctr);

    }
    printf("Dispatching\n");
    tOutput.start(callback(&queue, &EventQueue::dispatch_forever));
    tOutput2.start(callback(&queue, &EventQueue::dispatch_forever));
    // tOutput.join();
    while(1);
}