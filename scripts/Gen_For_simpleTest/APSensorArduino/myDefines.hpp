#ifndef APSensorArduino_DATA_H
#define APSensorArduino_DATA_H
//defines constants specific to APSensorArduino#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 10
#define numFrames 1
#define node_numData 1

int32_t collect_airPressure();

#define dataCollectorsList collect_airPressure

#endif