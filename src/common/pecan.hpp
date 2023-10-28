#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"

#define Param_Count 20
#define MAX_SIZE_PACKET_DATA 8

enum PCAN_ERR{
    PACKET_TOO_BIG=-3,
    NOT_RECEIVED=-2,
    NOSPACE=-1,
    SUCCESS = 0,
    GEN_FAILURE = 1,
};

enum MATCH_TYPE{
    MATCH_EXACT=0,
    MATCH_SIMILAR=1,
};

struct CANPacket {
    int id;
    char data[MAX_SIZE_PACKET_DATA] = {0};
    char dataSize = 0;
};

// A single CANListenParam
struct CANListenParam {
    int listen_id;
    int (*handler)(CANPacket *);
    MATCH_TYPE mt;
};

struct PCANListenParamsCollection{
    CANListenParam arr[Param_Count];
    int size = 0;
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
int waitPackets(CANPacket * recv_pack, PCANListenParamsCollection * plpc);
int addParam(PCANListenParamsCollection * plpc, CANListenParam clp);

int sendPacket(CANPacket*  p);

int combinedID(int fn_id, int node_id);
// Only in use with sensor stuff
void setSensorID(CANPacket * p, char sensorId);

bool exact(int id, int mask);
bool similar(int id, int mask);
static bool (*matcher[2])(int, int) = {exact, similar};

// Write size bytes to the packet, accounting
// For Max Length
int writeData(CANPacket * p, char * dataPoint, int size);

#endif