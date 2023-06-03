#include <mbed.h>
#include "../common/pecan.hpp"
#include "mcu.h"

#define MAX_BUFFER_SIZE 32
#define MOTOR_ID 11

int receiveHandler(CANPacket * pack){
    static BufferedSerial serial_port(USBTX, USBRX);
    char buf[MAX_BUFFER_SIZE] = {0};
    UART_Packet *new_packet;
    int i;
    int32_t reformatted;

    printf("MCU received:");
    for(i=0; i<4; i++){
        (int32_t)pack->data[i] << (8*i);
    }
    reformatted = pack->data[1] | pack->data[2] | pack->data[3] | pack->data[3];
    new_packet->speedRPM = reformatted;
    serial_port.write(new_packet, sizeof(UART_Packet *));

    return SUCCESS;
}

int main()
{
    // CANPACKET p;
    // char hello[] = "Shynn";
    printf("MCU startup \n");
    CAN can2(p30, p29, 500E3);

    CANPacket p;
    int out;
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
