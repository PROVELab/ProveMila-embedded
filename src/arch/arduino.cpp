#include "Arduino.h"
#include "../common/pecan.hpp"
#include "CAN.h"
#include <TaskScheduler.h>
typedef Task ard_event;

int16_t waitPacket(CANPacket * recv_pack, int16_t listen_id, int16_t (*handler)(CANPacket *)){
    if (recv_pack == NULL){
        CANPacket p;
        recv_pack = &p;
        // We can only use this for handler
        // Because stack
    }
    
    int16_t packetsize = CAN.parsePacket();
    int8_t id;
    if (!packetsize &&
        (id = CAN.packetId()) == listen_id
    ){
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        for (int8_t i = 0; i < packetsize; i++){
            recv_pack->data[i] = CAN.read();
        }
        return handler(recv_pack);
    }
    return NOT_RECEIVED;
}


int16_t sendPacket(CANPacket * p){
    if (p->dataSize > MAX_SIZE_PACKET_DATA){
        return PACKET_TOO_BIG;
    }
    CAN.beginPacket(p->id);
    for (int8_t i = 0; i < p->dataSize; i++){
        CAN.write( p->data[i]);
    }
    if(CAN.endPacket()){
        Serial.println("Success!!!");
    }
    return SUCCESS;
}

void PScheduler::mainloop(int8_t * inp){
    Scheduler ts;
    Task * tasks = (Task *) inp;
    Task tss[20] ={};
    for (int i = 0; i < this->ctr; i++){
        tss[i].set(
            this->tasks[i].interval,
            TASK_FOREVER,
            this->tasks[i].function
        );
    }
    ts.enableAll();
}