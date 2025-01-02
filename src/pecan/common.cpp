#include <stdint.h>
#include <string.h> // memcpy
#include "pecan.h"

uint32_t combinedID(uint32_t fn_id, uint32_t node_id){
    return (fn_id << 7) + node_id;
}

void setSensorID(struct CANPacket * p, uint8_t sensorId){
    p->data[0] = sensorId;
}

int16_t addParam(struct PCANListenParamsCollection * plpc, struct CANListenParam clp){
    if (plpc->size + 1 > MAX_PCAN_PARAMS){
        return NOSPACE;
    } else {
        plpc->arr[plpc->size] = clp;
        plpc->size++;
        return SUCCESS;
    }
}
int16_t setRTR(struct CANPacket * p){   //makes the given packet an RTR packet
    if(p->dataSize !=0){
        return 1;   //this packet has data written to it, it cant also be an rtr packet 
    }
    p->rtr=1;
    return 0;
}
int16_t writeData(struct CANPacket * p, int8_t * dataPoint, int16_t size){
    if(p->rtr){
        return -4;  //this is an rtr packet, can not write data
    }
    int16_t current_size = p->dataSize;
    int16_t i = 0;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }
    for (; current_size + i < current_size + size; i++){
        // DataSize can be interpreted as both
        // Size, and Index
        // Casting to 16-bit because compiler not happy
        p->data[(int16_t)p->dataSize] = dataPoint[i];
        p->dataSize++;

        // This check should've been working above
        // But just in case, we'll do it in the loop as well
        if (i > MAX_SIZE_PACKET_DATA){
            return NOSPACE;
        }
    }
    return SUCCESS;
}

bool exact(uint32_t id, uint32_t mask) {  // does not check extended bits of Id
    return (id & 0b11111111111) == mask;
}

//note: ID sent over CAN is 11 bit long, with first 7 bitsbeing the identifier of sending node, and last 4 bits being the function code
bool matchID(uint32_t id, uint32_t mask){ //check if the 7 bits of node ID must match mask
    return  getNodeId(id) == getNodeId(mask);
}

bool matchFunction(uint32_t id,uint32_t mask){    //mask should contain 4 bit functoin code in bits 7-10.
    return getFunctionId(id) == getFunctionId(mask);//only compares the functionCodes
}

