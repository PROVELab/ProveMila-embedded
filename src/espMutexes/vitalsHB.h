#ifndef vitalsHB
#define vitalsHB
#include "../pecan/pecan.h"
#include <stdio.h>

int16_t recieveHeartbeat(CANPacket* message);
void sendHB(void* pvParameters);

#endif
