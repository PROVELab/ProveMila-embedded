#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFRame array from this node. It stores data to be sent, and info for how to send

struct dataPoint f0DataPoints [1]={
    {.bitLength=8, .min=-10, .max=200},
};

struct CANFrame myframes[numFrames] = {
    {.frameNumData = 1, .frequency = 400, .startingDataIndex=0, .dataInfo=f0DataPoints},
};
