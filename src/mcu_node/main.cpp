#include <mbed.h>
#include "../common/pecan.hpp"
#include "mcu.h"

#define MAX_BUFFER_SIZE 32
#define MOTOR_ID 11

int receiveHandler(CANPACKET * pack){
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
    CANPACKET p;
    p.id = 11;
    char hello[] = "Shynn";
    printf("MCU Sent \n");
    writeData(&p, hello, 5);
    if (waitPacket(&p, 00, receiveHandler) == NOT_RECEIVED){
        ;
    }

}
