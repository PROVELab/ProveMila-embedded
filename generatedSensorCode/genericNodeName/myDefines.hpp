#ifndef genericNodeName_DATA_H
#define genericNodeName_DATA_H
//defines constants specific to genericNodeName
//each sensor file gets one of these .h files#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 6
#define numFrames 2
#define numData 4

int32_t collect_temperature1();
int32_t collect_temperature1();
int32_t collect_temperature1();
int32_t collect_temperature1();

#define dataCollectorsList collect_temperature1, collect_temperature2, collect_temperature3, collect_airPressure

#endif