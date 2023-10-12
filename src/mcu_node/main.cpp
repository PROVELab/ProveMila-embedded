#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

int receiveHandler(CANPacket * pack){
    printf("MCU received:\n");
    printf("%d-%s.\n", (pack->id), (pack->data));
    return SUCCESS;
}

void the_func( int a){
    printf("Every 1s, time: %u\n", (unsigned int) time(NULL));
}

void the_func2(int a){
    printf("Every 0.2s, time: %u\n", (unsigned int) time(NULL));
}

int main() {
    char events[2*(9+4 +3)*sizeof(int)];
    printf("Begin\n");
    Scheduler s;
    set_time(0);
    
    Task t;
    t.function = the_func;
    t.delay = 0;
    t.interval = 1000;
    s.scheduleTask(t);

    Task t2;
    t2.function = the_func2;
    t.delay = 0;
    t2.interval = 200;
    s.scheduleTask(t2);

    s.mainloop(events);
}
