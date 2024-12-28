#ifndef PECAN_H
#define PECAN_H
#include <stdint.h>
#include "./ptypes.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"

uint32_t combinedID(uint32_t fn_id, uint32_t node_id);


// Various PCAN Error Code return values
enum PCAN_ERR {
    PACKET_TOO_BIG = -3,
    NOT_RECEIVED = -2,
    NOSPACE = -1,
    SUCCESS = 0,
    GEN_FAILURE = 1,
};

// Exact match or Similar match (with mask)
enum MATCH_TYPE {
    MATCH_EXACT = 0,
    MATCH_ID = 1,
    MATCH_FUNCTION =2,
};

// A CANPacket: takes in 11-bit id, 8 bytes of data
struct CANPacket {
    uint32_t id;
    uint8_t data[MAX_SIZE_PACKET_DATA];
    uint8_t dataSize;
};
// A single CANListenParam
struct CANListenParam {
    int16_t listen_id;
    int16_t (*handler)(struct CANPacket*);
    enum MATCH_TYPE mt;
};



// The default handler for a packet who
// we couldn't match with other params
int16_t defaultPacketRecv(struct CANPacket* p);

// A collection containing an array of params to listen for
// and the default handler if none of the params match with
// a given packet
struct PCANListenParamsCollection {
    struct CANListenParam arr[MAX_PCAN_PARAMS];
    int16_t (*defaultHandler)(struct CANPacket* p);
    int16_t size;
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

int16_t waitPackets(struct CANPacket* recv_pack,struct PCANListenParamsCollection* plpc);

/// Sends a CANPacket p
int16_t sendPacket(struct CANPacket* p);

/// Adds a CANListenParam to the collection
int16_t addParam(struct PCANListenParamsCollection* plpc, struct CANListenParam clp);

// Only in use with sensor stuff
void setSensorID(struct CANPacket* p, uint8_t sensorId);

//used to run a task one Time

// Returns true if id == mask
bool exact(uint32_t id, uint32_t mask);
// Returns true if id matches the bits of mask
bool matchID(uint32_t id, uint32_t mask);
// Returns true if the functionCode of id ==mask
bool matchFunction(uint32_t id, uint32_t mask);

static bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};

// Write size bytes to the packet, accounting
// For Max Length
int16_t writeData(struct CANPacket* p, int8_t* dataPoint, int16_t size);
#endif