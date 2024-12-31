#include "Arduino.h"

#include <TaskScheduler.h>

#include "../common/pecan.hpp"
#include "../arch/arduino.hpp"
#include "CAN.h"

#include <Arduino.h>

Task dTasks [MAX_TASK_COUNT]={};    /*only one of these arrays is declared, there should only ever be one pScheduler instance, unless implementation is changed
to declare the array for each pScheduler instance inside main.cpp file.
*/

typedef Task ard_event;

int16_t defaultPacketRecv(CANPacket *packet) {  //this is only to be used by vitals for current
    Serial.print("Default Func: id ");
    Serial.print(packet->id);
    Serial.print(",  data ");
    Serial.println((char*) packet->data);
    return 1;
}

int16_t waitPackets(CANPacket *recv_pack, PCANListenParamsCollection *plpc) {
    //Serial.println(plpc->arr[0].listen_id);
    if (recv_pack == NULL) {
        CANPacket p;
        recv_pack = &p;
        // We can only use this for handler
        // Because stack
    }

    int8_t packetsize;
    CANListenParam clp;
    int32_t id;
    //Serial.print("packetSize: ");
    //Serial.println(CAN.parsePacket());
    if ((packetsize = CAN.parsePacket())) {
        recv_pack->id = CAN.packetId();
        recv_pack->dataSize = packetsize;
        recv_pack->rtr= CAN.packetRtr();
        // Read and temporarily store all the packet data into PCAN
        // packet (on stack memory)
        for (int8_t j = 0; j < recv_pack->dataSize; j++) {
            recv_pack->data[j] = CAN.read();
        }
        // Then match the packet id with our params; if none
        // match, use default handler
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

int16_t sendPacket(CANPacket *p) {  //note: if your id is longer than 11 bits it made into
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        return PACKET_TOO_BIG;
    }
    if(p->id>0b11111111111){
        CAN.beginExtendedPacket(p->id, p->dataSize, p->rtr);
    }else{
        CAN.beginPacket(p->id, p->dataSize, p->rtr);
    }
    for (int8_t i = 0; i < p->dataSize; i++) {
        CAN.write(p->data[i]);
    }
    if (CAN.endPacket()) {
        //Serial.print(CAN.parsePacket());
        //Serial.println(CAN.packetId());
    } else {
        return GEN_FAILURE;
    }
    return SUCCESS;
}
PScheduler::PScheduler(){};
/**
 * @brief 
 * 
 * @param inp Just pass in null, it's not used; this will allocate
 *              20 stuff on stack by itself 
 */
/*
void PScheduler::print(int num){
    Serial.println(num);
}*/
int PScheduler::scheduleTask(PTask *t){
    if (ctr >= MAX_TASK_COUNT){
        return NOSPACE;
    }
    //Task newTask;
    dTasks[ctr].set(t->interval,TASK_FOREVER,t->function);
    ctr++;
    //Serial.println(ctr);
    return ctr-1;
    }

void PScheduler::runOneTimeTask(int task, int timeDelay){
        //Serial.println(task);
        dTasks[task].disable();
        dTasks[task].setIterations(1);
        dTasks[task].enableDelayed(timeDelay);
    }

int PScheduler::scheduleOneTimeTask(PTask *t){    //only needs the function, delay and interval not used, the delay wanted is passed into runTask
        if (ctr >= MAX_TASK_COUNT){
        return NOSPACE;
    }
    //Task newTask;
    dTasks[ctr].set(1000,1,t->function);   //1000 chosen as default time if no time is indicated in 1 time task
    ctr++;
    return ctr-1;
}
//void PScheduler::mainloop(int8_t *inp)
void PScheduler::mainloop(PCANListenParamsCollection* listens) {
    
    PCANListenParamsCollection* inp=(PCANListenParamsCollection*) listens;
    Scheduler ts;
    for (int16_t i = 0; i < this->ctr; i++) {
        ts.addTask(dTasks[i]);
    }
    ts.enableAll();
    //while (1);
    CANPacket recv_pack;
    while(1){
        ts.execute();
        waitPackets(recv_pack,inp);
    }

}
