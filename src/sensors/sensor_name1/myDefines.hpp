// #ifndef SENSOR_NAME1_DATA_H
// #define SENSOR_NAME1_DATA_H
# pragma once
#define myId 6
#define numFrames 2
#define numData 4
//defines constants specific to <name1>
//each sensor file gets one of these .h files
#include "../common/sensorHelper.hpp"

//#include "sensorStaticDec.cpp"
#include<stdint.h>


//constexpr int numFrames = 4;


int32_t collectData1();     //TODO for final imple: replace with collecteData_<name>
int32_t collectData2();
int32_t collectData3();
int32_t collectData4();

#define dataCollectorsList collectData1, collectData2, collectData3, collectData4


//#endif
