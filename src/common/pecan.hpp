#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"

#define Param_Count 20
#define MAX_SIZE_PACKET_DATA 8

// Various PCAN Error Code return values
enum PCAN_ERR{
    PACKET_TOO_BIG=-3,
    NOT_RECEIVED=-2,
    NOSPACE=-1,
    SUCCESS = 0,
    GEN_FAILURE = 1,
};

// Exact match or Similar match (with mask)
enum MATCH_TYPE{
    MATCH_EXACT=0,
    MATCH_SIMILAR=1,
};

// A CANPacket: takes in 11-bit id, 8 bytes of data
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

// The default handler for a packet who
// we couldn't match with other params
int defaultPacketRecv(CANPacket *);

// A collection containing an array of params to listen for
// and the default handler
struct PCANListenParamsCollection{
    CANListenParam arr[Param_Count];
    int (*defaultHandler)(CANPacket *) = defaultPacketRecv;
    int size = 0;
};

/// @brief Blocking wait on a packet with listen_id, other packets are ignored
/// @param recv_pack a pointer to a packet-sized place in 
///                  such that the caller can see what
///                  the received packet is if necessary
///                  If NULL, the recv_pack will create its own
/// @param plpc A PCANListenParamsCollection, which specifies a bunch
///                  of ids to listen for, and their corresponding 
///                  handler functions
/// @return 0 on success, nonzero on Failure (see PCAN_ERR enum)
int waitPackets(CANPacket * recv_pack, PCANListenParamsCollection * plpc);

/// Adds a CANListenParam to the collection
int addParam(PCANListenParamsCollection * plpc, CANListenParam clp);

/// Sends a CANPacket p
int sendPacket(CANPacket* p);

// Combines a function id and node id into a full 11-bit id
int combinedID(int fn_id, int node_id);

// Only in use with sensor stuff
void setSensorID(CANPacket * p, char sensorId);

// Returns true if id == mask
bool exact(int id, int mask);
// Returns true if id matches the bits of mask
bool similar(int id, int mask);
static bool (*matcher[2])(int, int) = {exact, similar};

// Write size bytes to the packet, accounting
// For Max Length
int writeData(CANPacket * p, char * dataPoint, int size);

#endif