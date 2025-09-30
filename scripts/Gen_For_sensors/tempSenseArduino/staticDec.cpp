#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFrame array from this node. It stores data to be sent, and info for how to send

dataPoint f0DataPoints [3]={
    {.bitLength=7, .min=-10, .max=117},
    {.bitLength=32, .min=-2000000000, .max=2000000000},
    {.bitLength=20, .min=-524288, .max=524287},
};

dataPoint f1DataPoints [1]={
    {.bitLength=8, .min=-10, .max=200},
};

CANFrame myframes[numFrames] = {
    {.numData = 3, .frequency = 700, .startingDataIndex=0, .dataInfo=f0DataPoints},
    {.numData = 1, .frequency = 400, .startingDataIndex=3, .dataInfo=f1DataPoints},
};
