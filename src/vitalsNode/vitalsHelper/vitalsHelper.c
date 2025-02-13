#define startingOffset 6
#include "../../pecan/pecan.h"
#include <stdint.h>

#define HBFlag 0b1  //bit one stores if HB has been sent

uint32_t IDTovitalsIndex(uint32_t nodeID){
    uint32_t baseID= getNodeId(nodeID)-startingOffset;
    //loop over excluded

    return baseID;
} //returns which index of vitalsArray a node corresponds to

int32_t isolateBits(uint8_t* value, int8_t startingIndex, int8_t numBits){

    int32_t mask= 1U << (numBits - 1);    //Ex 7 -> 0b111111
    return (*((uint64_t*)value) >> startingIndex) & mask;
}

