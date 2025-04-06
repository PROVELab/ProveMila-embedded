#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFrame array from this node. It stores data to be sent, and info for how to send

dataPoint f0DataPoints [1]={
    {.bitLength=8, .min=-10, .max=200},
};

CANFrame myframes[numFrames] = {
    {.numData = 1, .frequency = 400, .startingDataIndex=0, .dataInfo=f0DataPoints},
};
