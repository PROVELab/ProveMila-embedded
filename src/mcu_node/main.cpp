#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;


int receiveHandler(CANPacket * pack){
    printf("MCU received from Telemetry:\n");
    printf("%d-%s\n", (pack->id), (pack->data));
    return SUCCESS;
}

int main()
{
    // can1.reset();
    //  p;
    // char hello[] = "Shynn";
    
    // CANMessage cm;
    // CANMessage p;
    // cm.data[0] = 'h';
    // cm.id = 1;
    // cm.len = 1;
    // for (int i = 0; i < 5; i++){
    //     printf("Send: %d\n", can1.write(cm));
    //     printf("Recv: %d\n", can1.read(p));
    //     printf("Received id: %d %s\n", p.id, p.data);
    // }


    printf("MCU startup \n");

    PCANListenParamsCollection plpc;
    CANListenParam clp;

    clp.handler = receiveHandler;
    clp.listen_id = combinedID(0b1111, 0b0);
    clp.mt = MATCH_EXACT;

    if (addParam(&plpc, clp) != SUCCESS){
        printf("No Space!");
        printf("Error\n");
        return -1;
    }

    CANPacket p;
    while(1){

        printf("Waiting\n");

        while (waitPackets(&p, &plpc) == NOT_RECEIVED){
        }
    }
}
