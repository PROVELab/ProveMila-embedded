#include <stdio.h>
#include <stdint.h>
#include "vitalsStaticDec.h"
#include "vitalsStructs.h"

#define R10(x) {x,x,x,x,x,x,x,x,x,x}
// Node 0: pedalSensor
dataPoint n0f0DPs [2]={
    {.bitLength=8, .minCritical=5, .maxCritical=95, .min=-20, .max=120, .minWarning=20, .maxWarning=80, .startingValue=50},
    {.bitLength=8, .minCritical=5, .maxCritical=95, .min=-20, .max=120, .minWarning=20, .maxWarning=80, .startingValue=50},
};

int32_t n0f0Data[2][10]={R10(50),R10(50)};

CANFrame n0[1]={
    {.nodeID=8, .frameID=0, .numData=2, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n0f0Data , .dataInfo=n0f0DPs},
};

// Node 1: APSensorESP
dataPoint n1f0DPs [1]={
    {.bitLength=32, .minCritical=5, .maxCritical=95, .min=-2147483648, .max=2147483647, .minWarning=20, .maxWarning=80, .startingValue=50},
};

int32_t n1f0Data[1][10]={R10(50)};

CANFrame n1[1]={
    {.nodeID=10, .frameID=1, .numData=1, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1500, .data=n1f0Data , .dataInfo=n1f0DPs},
};

// vitalsData *nodes;
vitalsNode nodes [2]={
    {.flags=0, .milliSeconds=0, .numFrames=1, .CANFrames=n0},
    {.flags=0, .milliSeconds=0, .numFrames=1, .CANFrames=n1},
};
int16_t missingIDs[]={9};
