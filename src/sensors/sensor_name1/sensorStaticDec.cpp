#include "myDefines.hpp"

#include "../common/sensorHelper.hpp"
//defines info for sending, and space for storing data for <name1>

//creates CANFRame array from this node. It stores data to be sent, and info for how to send

struct dataPoint f0DataPoints [3]={
    {7, -10,117, 0}, 
    {30, -536870912, 536870911, 0},
    {20, -524288, 524287, 0}
}; 
struct dataPoint f1DataPoints [1]={
    {4, -1, 12, 0}
};
struct CANFrame myframes[numFrames]= {//stores data that will be sent via CAN, and info on how to send data.
    {3, 0, f0DataPoints, 500},
    {1, 3, f1DataPoints, 250}
    }; 


