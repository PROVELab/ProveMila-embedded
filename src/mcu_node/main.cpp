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
    // // CANPACKET p;
    // // char hello[] = "Shynn";
    // printf("MCU startup \n");
    // CAN can2(p30, p29, 500E3);

    // CANPacket p;
    // int out;
    // while(1){
    //     // p.id = 15;
    //     // p.dataSize = 0;
    //     // writeData(&p, hello, 5);
    //     // printf("%d\n", sendPacket(&p));
    //     // if ((out = can2.read(msg))){
    //     //     printf("Msg len: %d\n", msg.len);
    //     // }
    //     while (waitPacket(&p, 11, receiveHandler) == NOT_RECEIVED){
    //     }
    // }
}
