#include <mbed.h>
#include "../common/pecan.hpp"
#include <rtos.h>
Thread thread;

int receiveHandler(CANPacket * pack){
    printf("MCU received from Telemetry: ");
    printf("%d-%s\n", (pack->id), (pack->data));
    printf("\n");
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


    printf("Booting up MCU\n");


    while (1){
        CANPacket  p;
        p.dataSize = 8;
        p.id = combinedID(0b1111, 0b0);
        strcpy( p.data, "hi lin!");
        printf("Sending\n");
        if (sendPacket(&p) == GEN_FAILURE){
            printf(":(\n");
        }

        p.dataSize = 8;
        p.id = combinedID(0b1110, 0b0);
        strcpy( p.data, "bye lin");
        printf("Sending\n");
        if (sendPacket(&p) == GEN_FAILURE){
            printf(":(\n");
        }
        ThisThread::sleep_for(4s);
    }

}
