#ifndef vitalsHelp
#define vitalsHelp
#include "../programConstants.h"

//fixed vitals Constants
#define HBFlag 0b1
// #define InitFlag 0b01
// #define invalidDataFrameFlag 0b001
// #define dataResetTimeout 0b1  //unable to restart timer for this data
// #define missingCanFrame 0b01

//#define numExcludedIDs 1    //if =0, below line ommitted
//int excludedIDs[numExcludedIDs] ={80}   //ex would exclude 80
uint32_t IDTovitalsIndex(uint32_t nodeID); //returns which index of vitalsArray a node corresponds to
uint32_t vitalsIndexToID(uint32_t nodeIndex);   //inverse of above
int32_t isolateBits(uint8_t* value, int8_t startingIndex, int8_t numBits);    //return an integer containing numBits  of value starting at startingIndex (bit-Indexed) . EX: (0b10101, 1, 3) -> 0b010
#endif