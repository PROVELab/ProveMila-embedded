#include <stdio.h>
#include <stdint.h>
//#include "programConstants.h"
#include "vitalsStaticDec.h"
#include "vitalsStructs.h"

#define R10(x) {x,x,x,x,x,x,x,x,x,x}
// Node 0: genericNodeName
struct dataPoint n0f0DPs [3]={
    {.bitLength=7, .isCritical=1, .minCritical=-5, .maxCritical=110, .min=-10, .max=117, .minWarning=0, .maxWarning=100, .startingValue=1},
    {.bitLength=32, .isCritical=1, .minCritical=-300000000, .maxCritical=300000000, .min=-2000000000, .max=2000000000, .minWarning=-100000000, .maxWarning=100000000, .startingValue=2},
    {.bitLength=20, .isCritical=1, .minCritical=0, .maxCritical=0, .min=-524288, .max=524288, .minWarning=-100000, .maxWarning=100000, .startingValue=3},
};

struct dataPoint n0f1DPs [1]={
    {.bitLength=32, .isCritical=1, .minCritical=0, .maxCritical=0, .min=-2147483648, .max=2147483647, .minWarning=1, .maxWarning=10, .startingValue=4},
};

int32_t n0f0Data[3][10]={R10(1),R10(2),R10(3)};

int32_t n0f1Data[1][10]={R10(4)};

struct CANFrame n0[2]={
    {.nodeID=6, .frameID=0, .numData=3, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n0f0Data , .dataInfo=n0f0DPs},
    {.nodeID=6, .frameID=1, .numData=1, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=600, .data=n0f1Data , .dataInfo=n0f1DPs},
};

// Node 1: sampleName
struct dataPoint n1f0DPs [1]={
    {.bitLength=8, .isCritical=1, .minCritical=-10, .maxCritical=110, .min=-10, .max=200, .minWarning=0, .maxWarning=110, .startingValue=9},
};

int32_t n1f0Data[1][10]={R10(9)};

struct CANFrame n1[1]={
    {.nodeID=8, .frameID=2, .numData=1, .flags=0, .dataLocation=0, .consecutiveMisses=0, .dataTimeout=1000, .data=n1f0Data , .dataInfo=n1f0DPs},
};

// struct vitalsData *nodes;
struct vitalsNode nodes [2]={
    {.flags=0, .milliSeconds=0, .numFrames=2, .CANFrames=n0},
    {.flags=0, .milliSeconds=0, .numFrames=1, .CANFrames=n1},
};
int16_t missingIDs[]={7};
