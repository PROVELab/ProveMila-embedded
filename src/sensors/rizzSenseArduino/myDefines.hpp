#ifndef rizzSenseArduino_DATA_H
#define rizzSenseArduino_DATA_H
//defines constants specific to rizzSenseArduino
//each sensor file gets one of these .h files#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 8
#define numFrames 1
#define node_numData 1

int32_t collect_rizzMeter();

#define dataCollectorsList collect_rizzMeter

#endif