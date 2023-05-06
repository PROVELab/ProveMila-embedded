#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"

enum PCAN_ERR{
    OUT_OF_SPACE = 0,
};

struct CANPACKET{
    int id;
    byte data[8];
    byte dataSize = 0;
};

struct CANLISTEN_PARAM{
    int size;
    int[] listen_ids;
    void (**handler)(void);
}

CANPACKET waitPacket(int listen_id, void (*handler)(void));
void sendPacket(CANPACKET p);
void setSensorID(CANPACKET * p, byte sensorId){
    p->data[0] = sensorId;
}

int writeData(CANPACKET * p, byte * dataPoint, int size){
    int i;
    for (i = 0; i < size; i++){
        // We can use dataSize as an index, which is why it's =
        if (p->dataSize >= MAX_SIZE_PACKET_DATA){
            p->data[p->dataSize+i] = dataPoint[i];
        }
        p->dataSize++;
    }
}

#endif