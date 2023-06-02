#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

int receiveHandler(CANPACKET * pack){
    printf("MCU received:\n");
    printf("%d-%s.\n", (pack->id), (pack->data));
    return SUCCESS;
}

CAN can2(p30, p29, 500E3);


int main()
{
    // CANPACKET p;
    // char hello[] = "Shynn";
    printf("MCU startup \n");

    int out;
    CANMessage msg;
    CANPACKET p;
    while(1){
        // p.id = 15;
        // p.dataSize = 0;
        // writeData(&p, hello, 5);
        // printf("%d\n", sendPacket(&p));
        // if ((out = can2.read(msg))){
        //     printf("Msg len: %d\n", msg.len);
        // }
        while (waitPacket(&p, 11, receiveHandler) == NOT_RECEIVED){
        }
    }
}
