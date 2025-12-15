#ifndef vitalsData
#define vitalsData
#include "../pecan/pecan.h"
#include <stdio.h>

int16_t monitorData(CANPacket* message);
void initializeDataTimers();

#endif
