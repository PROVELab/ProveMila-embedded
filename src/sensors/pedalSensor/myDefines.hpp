#ifndef pedalSensor_DATA_H
#define pedalSensor_DATA_H
// defines constants specific to pedalSensor#include "../common/sensorHelper.hpp"
#include <stdint.h>
#define myId         8
#define numFrames    1
#define node_numData 3

int32_t collect_pedalPowerReadingmV();
int32_t collect_pedalReadingOne();
int32_t collect_pedalReadingTwo();

#define dataCollectorsList collect_pedalPowerReadingmV, collect_pedalReadingOne, collect_pedalReadingTwo

#endif
