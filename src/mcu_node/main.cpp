#include <mbed.h>
#include "../common/pecan.hpp"

int receiveHandler(CANPACKET * pack){
    printf("MCU received:");
    printf("%s\n", (pack->data));
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
