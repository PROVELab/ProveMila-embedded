#include "../programConstants.h"
#include "../../pecan/pecan.h"
#include <stdint.h>
#include "vitalsStaticDec.h"
#include "vitalsHelper.h"

uint32_t IDTovitalsIndex(uint32_t nodeID){//returns which index of vitalsArray a node corresponds to
    uint32_t baseID= getNodeId(nodeID);
    //loop over excluded
    int16_t foundMisses=0;
    for(int i=0;missingIDs[i]<baseID && foundMisses<numMissingIDs;i++){
        // baseID--;
        foundMisses++;
    }
    return baseID - startingOffset - foundMisses;
} 

uint32_t vitalsIndexToID(uint32_t nodeIndex){   //inverse of above function
    uint32_t baseID= nodeIndex+startingOffset;
    //loop over excluded
    int16_t foundMisses=0;
    for(int i=0;missingIDs[i]<=baseID && foundMisses<numMissingIDs;i++){
        baseID++;
        foundMisses++;
    }
    return baseID;
} 

int32_t isolateBits(uint8_t* value, int8_t startingIndex, int8_t numBits){

    int32_t mask= 1U << (numBits - 1);    //Ex 7 -> 0b111111
    return (*((uint64_t*)value) >> startingIndex) & mask;
}

