#ifndef vitalsData
#define vitalsData
#include "../pecan/pecan.h"
#include <stdio.h>

int16_t moniterData(CANPacket* message);
void initializeDataTimers();

#endif
