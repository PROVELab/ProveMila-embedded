#include "../../sensors/common/sensorHelper.hpp"
#include "myDefines.hpp"

// creates CANFrame array from this node. It stores data to be sent, and info for how to send

dataPoint f0DataPoints[3] = {
    {.bitLength = 16, .min = -30000, .max = 35000},
    {.bitLength = 8, .min = -20, .max = 120},
    {.bitLength = 8, .min = -20, .max = 120},
};

CANFrame myframes[numFrames] = {
    {.numData = 3, .frequency = 100, .startingDataIndex = 0, .dataInfo = f0DataPoints},
};
