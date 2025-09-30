#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFrame array from this node. It stores data to be sent, and info for how to send

dataPoint f0DataPoints [2]={
    {.bitLength=8, .min=-20, .max=120},
    {.bitLength=8, .min=-20, .max=120},
};

CANFrame myframes[numFrames] = {
    {.numData = 2, .frequency = 400, .startingDataIndex=0, .dataInfo=f0DataPoints},
};
