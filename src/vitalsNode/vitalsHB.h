#ifndef vitalsHB
#define vitalsHB
#include <stdio.h>
#include "../pecan/pecan.h"   

int16_t recieveHeartbeat(CANPacket* message);
void sendHB( void * pvParameters );

#endif