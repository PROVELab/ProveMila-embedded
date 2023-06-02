#include "pecan.hpp"
// The general, independent implementation
int getID(int fn_id, int node_id){
    return (fn_id << 7) + node_id;
}

void setSensorID(CANPACKET * p, char sensorId){
    p->data[0] = sensorId;
}

int writeData(CANPACKET * p, char * dataPoint, int size){
    
    int i = p->dataSize;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }

    for (; i < size; i++){
        // DataSize can be interpreted as both
        // Size, and Index
        p->data[p->dataSize] = dataPoint[i];
        p->dataSize++;

        // This check should've been working above
        // But just in case, we'll do it in the loop as well
        if (i > MAX_SIZE_PACKET_DATA){
            return NOSPACE;
        }
    }
    return SUCCESS;
}