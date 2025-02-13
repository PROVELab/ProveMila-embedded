#include <stdio.h>
#include<stdint.h>
//#include "programConstants.h"
#include "vitalsStaticDec.h"
#include "vitalsStructs.h"
#define R10(x) {x,x,x,x,x,x,x,x,x,x}
    //note:ranges are -2^29 and 2^29 -1 to fit max amount into 2^30 range

//node 0: name
struct dataPoint n0f0DPs [3]={
    {7, -10,117, 0,100, 1,-5, 110}, 
    {30, -536870912, 536870911, -100000000, 100000000, 1, -300000000, -300000000},
    {20, -524288, 524287, -100000, 100000, 0, 0, 0}
}; 
struct dataPoint n0f1DPs [1]={
    {4, -1,12, 1,10, 0, 0, 0}
};
int32_t n0f0Data[3][10]={R10(1),R10(2), R10(3)};
int32_t n0f1Data[1][10]={R10(4)};


struct CANFrame n0[2]={
    {0,0, 3, 0,n0f0DPs, 0, n0f0Data,1000,0}, //1000 ms response time
    {0,1, 1, 0, n0f1DPs, 0, n0f1Data, 600, 0}
};

//node 1: name
struct dataPoint n1f0DPs [1]={
    {8, -10,200, 0,110, 1,-10, 110}
}; 
int32_t n1f0Data[1][10]={R10(5)};

struct CANFrame n1[1]={
    {1, 2, 1, 0,n1f0DPs, 0, n1f0Data,1000,0}, 
};
//

// struct vitalsData *nodes;
struct vitalsNode nodes [2]={
{0,0,2, n0},
{0,0,1,n1}
};