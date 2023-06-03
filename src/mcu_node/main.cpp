#include <mbed.h>
#include "../common/pecan.hpp"
#include "mcu.h"

#define MAX_BUFFER_SIZE 32
#define MOTOR_ID 11

CAN can1(p30, p29, 500E3);

int receiveHandler(CANPacket * pack){
    printf("MCU received:");
    static BufferedSerial serial_port(USBTX, USBRX);
    char buf[MAX_BUFFER_SIZE] = {0};
    UART_Packet *new_packet;
    int i;
    int32_t reformatted;

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
    can1.mode(CAN::LocalTest);
    // CANPACKET p;
    // char hello[] = "Shynn";
    printf("MCU startup \n");

    CANPacket p;
    int out;
    while(1){
        CANPacket  p, d;
        char hello[] = ":)\0";
        p.dataSize = 0;
        p.id = 11;

        writeData(&p, hello, 4);
        sendPacket(&p);
        printf("ReceiveHandler: %p\n", receiveHandler);
        while (waitPacket(&d, 11, receiveHandler) == NOT_RECEIVED){
            printf("SendPacket: %d\n", sendPacket(&p));
        }
    }
}
