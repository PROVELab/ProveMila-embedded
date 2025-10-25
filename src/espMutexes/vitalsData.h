#ifndef vitalsData
#define vitalsData
#include <stdio.h>
#include "../pecan/pecan.h"    

int16_t moniterData(CANPacket* message);
void initializeDataTimers();

#endif
