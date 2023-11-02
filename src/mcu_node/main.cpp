#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

CAN can1(p30, p29, 125000);

int receiveHandler(CANPacket * pack){
    printf("MCU received:\n");
    printf("%d-%s.\n", (pack->id), (pack->data));
    return SUCCESS;
}

int main()
{
    // CANPACKET p;
    // char hello[] = "Shynn";
    can1.mode(CAN::LocalTest);
    CANMessage cm;
    cm.data[0] = 'h';
    cm.id = 1;
    cm.len = 1;
    printf("Send: %d\n", can1.write(cm));
    printf("Recv: %d\n", can1.read(cm));


    // printf("MCU startup \n");

    // PCANListenParamsCollection plpc;
    // CANListenParam clp;

    // clp.handler = receiveHandler;
    // clp.listen_id = combinedID(0b1111, 0b0);
    // clp.mt = MATCH_EXACT;

    // if (addParam(&plpc, clp) != SUCCESS){
    //     printf("No Space!");
    //     printf("Error\n");
    //     return -1;
    // }

    // CANPacket p;
    // CANMessage cm;
    // p.id = clp.listen_id;
    // p.data[0] = 'h';
    // p.data[1] = '\0';
    // p.dataSize = 2;
    // can1.reset();
    // printf("Mode change: %d\n", can1.mode(CAN::LocalTest));
    // for (int i = 0; i < 5; i++){
    //     if (can1.write(CANMessage(p.id, p.data, p.dataSize))){
    //         printf("%d, %d\n", can1.rderror(), can1.tderror());
    //         printf("Receiving: %d\n", can1.read(cm));
    //     }
    // }
    // while(1){

    //     printf("Waiting\n");
    //     printf("%d\n", sendPacket(&p));

    //     while (waitPackets(&p, &plpc) == NOT_RECEIVED){
    //     }
    // }
}
