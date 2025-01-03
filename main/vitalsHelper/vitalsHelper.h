#ifndef vitalsHelp
#define vitalsHelp

#define HBFlag 0b1
#define InitFlag 0b01
#define invalidDataFrameFlag 0b001


//#define numExcludedIDs 1    //if =0, below line ommitted
//int excludedIDs[numExcludedIDs] ={80}   //ex would exclude 80
uint32_t IDTovitalsIndex(uint32_t nodeID); //returns which index of vitalsArray a node corresponds to
int32_t isolateBits(uint8_t* value, int8_t startingIndex, int8_t numBits);    //return an integer cotaining numBits bits of value starting at startingIndex (bit-Indexed) . EX: (0b10101, 1, 3) -> 0b010
#endif