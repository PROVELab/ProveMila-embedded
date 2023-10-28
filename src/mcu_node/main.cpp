#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

int receiveHandler(CANPacket * pack){
    printf("MCU received:\n");
    printf("%d-%s.\n", (pack->id), (pack->data));
    return SUCCESS;
}

int main()
{
    // CANPACKET p;
    // char hello[] = "Shynn";
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
        while (waitPackets(&p, &plpc) == NOT_RECEIVED){
        }
    }
}
