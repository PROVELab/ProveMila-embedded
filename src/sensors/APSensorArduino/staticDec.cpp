#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFrame array from this node. It stores data to be sent, and info for how to send

dataPoint f0DataPoints [1]={
    {.bitLength=32, .min=-2147483648, .max=2147483647},
};

CANFrame myframes[numFrames] = {
    {.numData = 1, .frequency = 700, .startingDataIndex=0, .dataInfo=f0DataPoints},
};
