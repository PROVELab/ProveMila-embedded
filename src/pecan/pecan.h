#ifndef PECAN_H
#define PECAN_H
#ifdef __cplusplus
extern "C" {  // Ensures C linkage for all functions. This is needed since arduino and common files are cpp, while esp Specific files are c, and pecan.h has function declarations for both. This will compile all functions with C linkage 
#endif

#include <stdint.h>
#include <stdbool.h>

//pytpes moved here:
#define MAX_SIZE_PACKET_DATA 8
// DEFINITIONS, TYPEDEFS
#define MAX_TASK_COUNT 5
#define MAX_PCAN_PARAMS 6
//

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
struct CANPacket {  //make sure to initialize using {0}
    uint8_t data[MAX_SIZE_PACKET_DATA];
    uint32_t id;
    uint8_t dataSize;
    int8_t rtr;
    int8_t extendedID;  //whether or not the packet is extended. 
};
// A single CANListenParam
struct CANListenParam {
    uint32_t listen_id;
    int16_t (*handler)(struct CANPacket*);
    enum MATCH_TYPE mt;
};


// A collection containing an array of params to listen for
// and the default handler if none of the params match with
// a given packet
struct PCANListenParamsCollection {
    struct CANListenParam arr[MAX_PCAN_PARAMS];
    int16_t (*defaultHandler)(struct CANPacket* p);
    int16_t size;
};

//common implementations:

/// Adds a CANListenParam to the collection
int16_t addParam(struct PCANListenParamsCollection* plpc, struct CANListenParam clp);

// Only in use with sensor stuff
void setSensorID(struct CANPacket* p, uint8_t sensorId);

uint32_t combinedID(uint32_t fn_id, uint32_t node_id);
uint32_t combinedIDExtended(uint32_t fn_id, uint32_t node_id, uint32_t extension);//also combines an extended ID

inline uint32_t getNodeId(uint32_t id){ //take the first seven bits of Can Id to isolate nodeId
    return id & 0b1111111;
}
inline uint32_t getFunctionId(uint32_t id){  //take bits 7-10 for functionId
    return (id>>7)&0b1111;
}
inline uint32_t getIdExtension(uint32_t id){  //take bits 7-10 for functionId
    return (id>>11) & 0b111111111111111111; //18 bit extension
}
inline uint32_t getDataFrameId(uint32_t id){
    return getIdExtension(id) & 0b11;   //the Can Frame index is stored in first two bits of extension
}

// Returns true if id == mask
bool exact(uint32_t id, uint32_t mask);
// Returns true if id matches the bits of mask
bool matchID(uint32_t id, uint32_t mask);
// Returns true if the functionCode of id ==mask
bool matchFunction(uint32_t id, uint32_t mask);
//bool (*matcher[3])(uint32_t, uint32_t);
//static bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};

// Write size bytes to the packet, accounting
// For Max Length
int16_t writeData(struct CANPacket* p, int8_t* dataPoint, int16_t size);


//void bitcpy(uint32_t* src, int8_t)
//makes a packet an RTR packet
int16_t setRTR(struct CANPacket * p);
//makes a packet an extended packet
int16_t setExtended(struct CANPacket * p);

//the below functions are used in base implementation of vitals and sensors, and may be needed elsewhere:
int32_t squeeze(int32_t value, int32_t min, int32_t max);   //identical to arduino constrain macro, other mcs dont have it :(

uint32_t formatValue(int32_t value, int32_t min, int32_t max);  //returns value in sensors standard format for CAN data. (forces data in bounds, and makes it signed)

int16_t copyValueToData(uint32_t* value, uint8_t* target, int8_t startBit, int8_t numBits); //copies the first numBits of value into target, starting from target's startBit'th bit. target must be 8 bytes.

int16_t copyDataToValue(uint32_t* target, uint8_t* data, int8_t startBit, int8_t numBits);  //inverse of above function
//
//platform specific:
//


// The default handler for a packet who
// we couldn't match with other params
int16_t defaultPacketRecv(struct CANPacket* p); //only platform specific because it prints things. In final implementation, we won't need this to print, so could be merged

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

#ifdef __cplusplus
}  // End extern "C"
#endif

#endif