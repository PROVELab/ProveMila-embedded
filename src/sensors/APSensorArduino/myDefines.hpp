#ifndef APSensorESP_DATA_H
#define APSensorESP_DATA_H
// defines constants specific to APSensorESP#include "../common/sensorHelper.hpp"
#include <stdint.h>
#define myId         10
#define numFrames    1
#define node_numData 1

int32_t collect_airPressure();

#define dataCollectorsList collect_airPressure

#endif
