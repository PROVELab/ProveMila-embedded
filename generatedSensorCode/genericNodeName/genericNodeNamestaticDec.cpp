#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFrame array from this node. It stores data to be sent, and info for how to send

struct dataPoint f0DataPoints [2]={
    {.bitLength=7, .min=-10, .max=117},
    {.bitLength=30, .min=-536870912, .max=536870911},
    {.bitLength=20, .min=-524288, .max=524288},
};

struct dataPoint f1DataPoints [2]={
    {.bitLength=4, .min=-1, .max=12},
};

struct CANFrame myframes[numFrames] = {
    {.frameNumData = 3, .frequency = 500, .startingDataIndex=0, .dataInfo=f0DataPoints},
    {.frameNumData = 1, .frequency = 100, .startingDataIndex=3, .dataInfo=f1DataPoints},
};
