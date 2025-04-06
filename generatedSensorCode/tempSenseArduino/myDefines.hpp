#ifndef tempSenseArduino_DATA_H
#define tempSenseArduino_DATA_H
//defines constants specific to tempSenseArduino
//each sensor file gets one of these .h files#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 9
#define numFrames 2
#define node_numData 4

int32_t collect_temperature1();
int32_t collect_temperature2();
int32_t collect_temperature3();
int32_t collect_rizzMeter();

#define dataCollectorsList collect_temperature1, collect_temperature2, collect_temperature3, collect_rizzMeter

#endif