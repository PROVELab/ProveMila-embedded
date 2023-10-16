#include "pecan.hpp"

// The general, independent implementation
int16_t getID(int16_t fn_id, int16_t node_id){
    return (fn_id << 7) + node_id;
}

void setSensorID(CANPacket * p, uint8_t sensorId){
    p->data[0] = sensorId;
}

int16_t writeData(CANPacket * p, int8_t * dataPoint, int16_t size){
    
    int16_t i = p->dataSize;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }

    for (; i < size; i++){
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
