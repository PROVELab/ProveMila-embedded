#include "myDefines.hpp"
#include "../common/sensorHelper.hpp"

//creates CANFRame array from this node. It stores data to be sent, and info for how to send

struct dataPoint f0DataPoints [3]={
    {.bitLength=7, .min=-10,.max=117, .data=0}, 
    {.bitLength=30, .min=-536870912, .max=536870911, .data=0},
    {.bitLength=20, .min=-524288, .max=524287, .data=0}
}; 
struct dataPoint f1DataPoints [1]={
    {.bitLength=4, .min=-1, .max=12, .data=0}
};
struct CANFrame myframes[numFrames]= {//stores data that will be sent via CAN, and info on how to send data.
    {.frameNumData=3, .startingDataIndex=0, .dataInfo=f0DataPoints, .frequency=500},
    {.frameNumData=1, .startingDataIndex=3, .dataInfo=f1DataPoints, .frequency=250}
}; 


