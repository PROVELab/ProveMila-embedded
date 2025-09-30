#include <stdio.h>
#include <stdint.h>
//#include "programConstants.h"
#include "vitalsStaticDec.h"
#include "vitalsStructs.h"

#define R10(x) {x,x,x,x,x,x,x,x,x,x}
// Node 0: tempSenseESP
dataPoint n0f0DPs [3]={
    {.bitLength=7, .minCritical=-5, .maxCritical=110, .min=-10, .max=117, .minWarning=0, .maxWarning=100, .startingValue=1},
    {.bitLength=32, .minCritical=-300000000, .maxCritical=300000000, .min=-2000000000, .max=2000000000, .minWarning=-100000000, .maxWarning=100000000, .startingValue=2},
    {.bitLength=20, .minCritical=-524288, .maxCritical=524287, .min=-524288, .max=524287, .minWarning=-100000, .maxWarning=100000, .startingValue=3},
};

dataPoint n0f1DPs [1]={
    {.bitLength=32, .minCritical=-2147483648, .maxCritical=2147483647, .min=-2147483648, .max=2147483647, .minWarning=-2147483648, .maxWarning=10, .startingValue=-2147483648},
};

int32_t n0f0Data[3][10]={R10(1),R10(2),R10(3)};

int32_t n0f1Data[1][10]={R10(-2147483648)};

CANFrame n0[2]={
    {.nodeID=6, .frameID=0, .numData=3, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n0f0Data , .dataInfo=n0f0DPs},
    {.nodeID=6, .frameID=1, .numData=1, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=600, .data=n0f1Data , .dataInfo=n0f1DPs},
};

// Node 1: rizzSenseArduino
dataPoint n1f0DPs [1]={
    {.bitLength=8, .minCritical=-10, .maxCritical=110, .min=-10, .max=200, .minWarning=0, .maxWarning=110, .startingValue=9},
};

int32_t n1f0Data[1][10]={R10(9)};

CANFrame n1[1]={
    {.nodeID=8, .frameID=2, .numData=1, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n1f0Data , .dataInfo=n1f0DPs},
};

// Node 2: tempSenseArduino
dataPoint n2f0DPs [3]={
    {.bitLength=7, .minCritical=-5, .maxCritical=110, .min=-10, .max=117, .minWarning=0, .maxWarning=100, .startingValue=1},
    {.bitLength=32, .minCritical=-300000000, .maxCritical=300000000, .min=-2000000000, .max=2000000000, .minWarning=-100000000, .maxWarning=100000000, .startingValue=2},
    {.bitLength=20, .minCritical=-524288, .maxCritical=524287, .min=-524288, .max=524287, .minWarning=-100000, .maxWarning=100000, .startingValue=3},
};

dataPoint n2f1DPs [1]={
    {.bitLength=8, .minCritical=-10, .maxCritical=110, .min=-10, .max=200, .minWarning=0, .maxWarning=110, .startingValue=9},
};

int32_t n2f0Data[3][10]={R10(1),R10(2),R10(3)};

int32_t n2f1Data[1][10]={R10(9)};

CANFrame n2[2]={
    {.nodeID=9, .frameID=3, .numData=3, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1500, .data=n2f0Data , .dataInfo=n2f0DPs},
    {.nodeID=9, .frameID=4, .numData=1, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n2f1Data , .dataInfo=n2f1DPs},
};

// Node 3: APSensorESP
dataPoint n3f0DPs [1]={
    {.bitLength=32, .minCritical=-2147483648, .maxCritical=2147483647, .min=-2147483648, .max=2147483647, .minWarning=-2147483648, .maxWarning=2147483647, .startingValue=2147483647},
};

int32_t n3f0Data[1][10]={R10(2147483647)};

CANFrame n3[1]={
    {.nodeID=10, .frameID=5, .numData=1, .isCritical=0, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1500, .data=n3f0Data , .dataInfo=n3f0DPs},
};

// vitalsData *nodes;
vitalsNode nodes [4]={
    {.flags=0, .milliSeconds=0, .numFrames=2, .CANFrames=n0},
    {.flags=0, .milliSeconds=0, .numFrames=1, .CANFrames=n1},
    {.flags=0, .milliSeconds=0, .numFrames=2, .CANFrames=n2},
    {.flags=0, .milliSeconds=0, .numFrames=1, .CANFrames=n3},
};
int16_t missingIDs[]={7};
