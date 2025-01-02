#ifndef SENSOR_NAME1_DATA_H
#define SENSOR_NAME1_DATA_H
//defines constants specific to <name1>
//each sensor file gets one of these .h files
#include "../common/sensorHelper.hpp"
#include<stdint.h>
#define myId 6
#define numFrames 2
#define numData 4


int32_t collectData1();     //TODO for final imple: replace with collecteData_<name>
int32_t collectData2();
int32_t collectData3();
int32_t collectData4();

#define dataCollectorsList collectData1, collectData2, collectData3, collectData4


#endif

//just use an effective #define nodeConstants.h <whichever file u want for node specific constants.
//then include this in every file This file mayhaps can now be converted into a general hpp
//included by every file(ei, migrated to sensorHelper???)