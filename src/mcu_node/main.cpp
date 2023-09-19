#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

int receiveHandler(CANPacket * pack){
    printf("MCU received:\n");
    printf("%d-%s.\n", (pack->id), (pack->data));
    return SUCCESS;
}

void the_func(){
    printf("Every 0.5s\n");
}

void the_func2(){
    printf("Every 1s\n");
}

int main() {
    printf("Begin\n");
    // Scheduler s;
    
    // Task t;
    // t.function = the_func;
    // t.interval = 500;
    // s.scheduleTask(t);
    // s.mainloop();
}
