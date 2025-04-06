#ifndef tempSenseESP_DATA_H
#define tempSenseESP_DATA_H
//defines constants specific to tempSenseESP
//each sensor file gets one of these .h files#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 6
#define numFrames 2
#define node_numData 4

int32_t collect_temperature1();
int32_t collect_temperature2();
int32_t collect_temperature3();
int32_t collect_airPressure();

#define dataCollectorsList collect_temperature1, collect_temperature2, collect_temperature3, collect_airPressure

#endif