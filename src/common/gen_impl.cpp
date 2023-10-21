#include "pecan.hpp"
// The general, independent implementation
int combinedID(int fn_id, int node_id){
    return (fn_id << 7) + node_id;
}

void setSensorID(CANPacket * p, char sensorId){
    p->data[0] = sensorId;
}

int writeData(CANPacket * p, char * dataPoint, int size){
    
    int i = p->dataSize;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }

    for (; i < size; i++){
        // DataSize can be interpreted as both
        // Size, and Index
        p->data[(int)p->dataSize] = dataPoint[i];
        p->dataSize++;

        // This check should've been working above
        // But just in case, we'll do it in the loop as well
        if (i > MAX_SIZE_PACKET_DATA){
            return NOSPACE;
        }
    }
    return SUCCESS;
}

template <typename T>
void fillBuf(char * b, T value){
    union {
        T value;
        char buffer[sizeof(T)];
    } convert_o_tron;
    convert_o_tron.value = value;

    for (int i = sizeof(T)-1; i >= 0; i--){
        b[i] = convert_o_tron.buffer[i];
    }
}

template <typename T>
void unFillBuf(char * b, T * valRef){
    union {
        T value;
        char buffer[sizeof(T)];
    } convert_o_tron;
    for (int i = 0; i < sizeof(T); i++){
        convert_o_tron.buffer[i] = b[i];
    }
    *valRef = convert_o_tron.value;
}