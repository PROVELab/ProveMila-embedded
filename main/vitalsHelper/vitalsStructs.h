#ifndef VITALS_STRUCTS_H
#define VITALS_STRUCTS_H

#include <stdio.h>
#include<stdint.h>
#include "programConstants.h"
#include "vitalsStaticDec.h"
#define R10(x) {x,x,x,x,x,x,x,x,x,x}
struct dataPoint{   //could define similar non-critical or smaller (int8_t and int16_t and use a void* to declare dataInfo if space is an issue, as this is very space inefficient, but makes it so casting is not necessary in main file which is nice :)
    int8_t bitLength;   
    int32_t min;   
    int32_t max;
    int32_t minWarning; 
    int32_t maxWarning;
    int8_t isCritical;  //0 or 1, determines whether or not we should declare min/maxCritical values. For this struct should always be 1
    int32_t minCritical;
    int32_t maxCritical;
};

struct CANFrame{    //identified by a 2 bite identifier 0-3 in function code
    //constants
    int8_t nodeID;  //used to identify which node this frame corresponds in callback for missed frames. stores the actual CAN ID of node, not vital's index
    int8_t frameID; //used to identify which frame this is in callback for missed frames. This matches its index in missingDataTimers. This is not the same as the ID that recognizes this frame as a CAN Message. HQ will use this to identify which data is missing.
    int8_t numData; //number of data pieces in this frame
    int8_t flags;   //1. Unable to reset data timeout 2-7: awaiting definitoins :)
    struct dataPoint *dataInfo; 
    //
    //TODO: figure out dataTimeout
    //i dont think ^this is the right spot for this. timeout should just be baked into how often task runs checking, will need means of mapping based to all frames based on timeout
    //
    //non-constants, update through program
    int8_t dataLocation;    //current location for writing data in circular array (0-9)
    int32_t (*data) [10]; //initialized to [data points per data =10] [numData for this frame]
    int32_t dataTimeout;    //how Often vitals needs to recieve this frame without extrapolating or raising error. Should be at least 2x however often the data is sent. Will default this to 3x in python script
    int8_t consecutiveMisses;   //a data is "missed" if it is not sent within the window of dataTimeout
};

//stuff for Vitals future use, currently only used to track recent information about a node
struct vitalsNode{        
    int8_t flags;      //various status info. index correspondence: 0: HB sent. 1: node ran setup. 2: invalid data frame recieved from node with this id. 4-7: awaiting definitoins :)
    int16_t milliSeconds;   //stores time to respond to HB
    int8_t numFrames;
    struct CANFrame* CANFrames;
    //int *nestedptr;
};
#endif