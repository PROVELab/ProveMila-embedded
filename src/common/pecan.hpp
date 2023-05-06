#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"

enum PCAN_ERR{
    PACKET_TOO_BIG=-3,
    NOT_RECEIVED=-2,
    NOSPACE=-1,
    SUCCESS = 0,
    GEN_FAILURE = 1,
};

struct CANPACKET{
    uint16_t id;
    uint8_t data[8];
    uint8_t dataSize = 0;
};

// The idea is that we shouldn't
// be passing all of this by value
// again and again and again, rather
// we pass this struct by reference
// and its elements can be accessed
// in order to handle multiple listen_ids
struct CANLISTEN_PARAM{
    int size;
    int * listen_ids;
    int (**handler)(CANPACKET *);
};


/// @brief Blocking wait on a packet with listen_id, other packets are ignored
/// @param recv_pack a pointer to a packet-sized place in 
///                  such that the caller can see what
///                  the received packet is if necessary
///                  If NULL, the recv_pack will create its own
/// @param listen_id the integer id we're listening for
/// @param handler a function that takes in a
///                pointer to the received packet and returns int
///                for success/failure
/// @return 0 on success, nonzero on Failure (see PCAN_ERR enum)
int waitPacket(CANPACKET * recv_pack, int listen_id, int (*handler)(CANPACKET *));
int waitPackets(CANPACKET * recv_pack, CANLISTEN_PARAM * params); // Pass by Ref

int sendPacket(CANPACKET*  p);

int getID(int fn_id, int node_id){
    return fn_id << 7 + node_id;
}
// Only in use with sensor stuff
void setSensorID(CANPACKET * p, byte sensorId){
    p->data[0] = sensorId;
}

// Write size bytes to the packet, accounting
// For Max Length
int writeData(CANPACKET * p, byte * dataPoint, int size){
    
    int i = p->dataSize;
    if (i + size > MAX_SIZE_PACKET_DATA){
        return NOSPACE;
    }

    for (; i < size; i++){
        // DataSize can be interpreted as both
        // Size, and Index
        p->data[p->dataSize+i] = dataPoint[i];
        p->dataSize++;

        // This check should've been working above
        // But just in case, we'll do it in the loop as well
        if (i > MAX_SIZE_PACKET_DATA){
            return NOSPACE;
        }
    }
    return SUCCESS;
}

#endif