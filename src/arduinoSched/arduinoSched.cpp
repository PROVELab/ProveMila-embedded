#include "Arduino.h"

#include <TaskScheduler.h>

#include "../arduinoSched/arduinoSched.hpp"
#include "../pecan/pecan.h"
#include "CAN.h"

#include <Arduino.h>

Task dTasks[MAX_TASK_COUNT] =
    {}; /*only one of these arrays is declared, there should only ever be one
           pScheduler instance*/

typedef Task ard_event;

PScheduler::PScheduler() {}
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
int PScheduler::scheduleTask(PTask *t) {
    if (ctr >= MAX_TASK_COUNT) {
        return NOSPACE;
    }
    // Task newTask;
    dTasks[ctr].set(t->interval, TASK_FOREVER, t->function);
    ctr++;
    // Serial.println(ctr);
    return ctr - 1;
}

void PScheduler::runOneTimeTask(int task, int timeDelay) {
    // Serial.println(task);
    dTasks[task].disable();
    dTasks[task].setIterations(1);
    dTasks[task].enableDelayed(timeDelay);
}

int PScheduler::scheduleOneTimeTask(
    PTask *t) { // only needs the function, delay and interval not used, the
                // delay wanted is passed into runTask
    if (ctr >= MAX_TASK_COUNT) {
        return NOSPACE;
    }
    // Task newTask;
    dTasks[ctr].set(1000, 1, t->function); // 1000 chosen as default time if no
                                           // time is indicated in 1 time task
    ctr++;
    return ctr - 1;
}
// void PScheduler::mainloop(int8_t *inp)
void PScheduler::mainloop(PCANListenParamsCollection *listens) {

    PCANListenParamsCollection *inp = (PCANListenParamsCollection *)listens;
    Scheduler ts;
    for (int16_t i = 0; i < this->ctr; i++) {
        ts.addTask(dTasks[i]);
    }
    ts.enableAll();
    // while (1);
    CANPacket recv_pack;
    while (1) {
        ts.execute();
        waitPackets(&recv_pack, inp);
    }
}
