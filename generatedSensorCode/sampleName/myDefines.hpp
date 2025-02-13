#ifndef sampleName_DATA_H
#define sampleName_DATA_H
//defines constants specific to sampleName
//each sensor file gets one of these .h files#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 8
#define numFrames 1
#define numData 1

int32_t collect_rizzMeter();

#define dataCollectorsList collect_rizzMeter

#endif