#include <stdint.h>

#include "../../pecan/pecan.h"
#include "vitalsHelper.h"

#if defined(__cplusplus)
#define STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#else
#define STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif

static inline uint32_t mask_u32(unsigned bits) { return (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u); }

// Must ensure the ID/index is valid before calling either of these.
int32_t IDTovitalsIndex(uint32_t nodeID) { // returns which index of vitalsArray a node corresponds to
    uint32_t baseID = getNodeId(nodeID);
    // loop over excluded
    int16_t foundMisses = 0;
    for (int i = 0; missingIDs[i] < baseID && foundMisses < numMissingIDs; i++) { foundMisses++; }
    // Example: if we get 11, with base 6, two missing IDs, and 3 nodes
    // (id must be 6-10): 11 - 6 - 2 = 3. totalNumNodes =3, since >=, invalild
    int32_t nodeIndex = baseID - startingOffset - foundMisses;
    if (nodeIndex >= numberOfNodes || nodeIndex < 0) { return invalidVitalsIndex; }
    return nodeIndex;
}

uint32_t vitalsIndexToID(uint32_t nodeIndex) { // inverse of above function
    // CAN Frames have to store nodeID anyway. can use this as a shortcut
    if (nodes[nodeIndex].numFrames > 0) { return nodes[nodeIndex].CANFrames->nodeID; }
    // Otherwise compute based on missingID's
    uint32_t baseID = nodeIndex + startingOffset;
    // loop over excluded
    int16_t foundMisses = 0;
    for (int i = 0; missingIDs[i] <= baseID && foundMisses < numMissingIDs; i++) {
        baseID++;
        foundMisses++;
    }
    return baseID;
}

int32_t isolateBits(uint8_t* value, int8_t startingIndex, int8_t numBits) {
    if (value == NULL || startingIndex < 0 || numBits <= 0 || numBits > 32 || startingIndex + numBits > 64) {
        return 0;
    }
    const uint64_t data = *((uint64_t*) value);
    const uint64_t mask = ((uint64_t) 1u << numBits) - 1u; // create numBits-wide mask
    return (int32_t) ((data >> startingIndex) & mask);
}

// The last bit used is [warningDataFlagIndex + maxDataInFrameBits - 1]
STATIC_ASSERT(warningDataFlagIndex + maxDataInFrameBits <= 32,
              "Warning payload exceeds 32 bits; widen to uint64_t or reduce fields");
STATIC_ASSERT(warningFrameFlagIndex >= warningNodeFlagIndex, "warningFrameFlagIndex must be >= warningNodeFlagIndex");

// Helpers for sending warnings.
//  Layout (LSB -> MSB):
//  [ 0 .. warningNodeFlagIndex-1 ]           : flags (type + detail flags)
//  [ warningNodeFlagIndex .. warningFrameFlagIndex-1 ] : nodeID
//  [ warningFrameFlagIndex .. warningDataFlagIndex-1 ] : frameID (maxFrameCntBits)
//  [ warningDataFlagIndex .. +maxDataInFrameBits-1 ]   : datapoint index (set to 0)

// can set dataPointIndex=0 if not necessary (or to any value really, but 0 to be consistent)
// flags should be or'd with Critical or warning as well!
void sendWarningForDataPoint(const CANFrame* problemFrame, uint8_t dataPointIndex, uint32_t flags) {

    const uint32_t nodeIDBits = (uint32_t) (warningFrameFlagIndex - warningNodeFlagIndex);
    const uint32_t nodeIDMask = mask_u32(nodeIDBits);
    const uint32_t frameIDMask = mask_u32(maxFrameCntBits);
    const uint32_t dataIndexMask = mask_u32(maxDataInFrameBits);
    const uint32_t flagsMask = mask_u32(warningNodeFlagIndex);

    uint32_t data = 0;

    // Flags (includes nonCritical/Critical + any detail flags), clipped to the flags region
    data |= flags & flagsMask;
    // Node ID
    data |= (problemFrame->nodeID & nodeIDMask) << warningNodeFlagIndex;
    // Frame ID
    data |= (problemFrame->frameID & frameIDMask) << warningFrameFlagIndex;
    // Datapoint index (can just set to 0 if irelevant)
    data |= (dataPointIndex & dataIndexMask) << warningDataFlagIndex;

    // Compute payload size
    const uint32_t totalBits = warningDataFlagIndex + maxDataInFrameBits;
    const uint8_t payloadSize = (uint8_t) ((totalBits + 7u) >> 3);

    // send the warning
    CANPacket message = {0};
    message.id = combinedID(warningCode, vitalsID);
    writeData(&message, (int8_t*) &data, payloadSize); // relies on both being little-endian
    sendPacket(&message);
}

void sendWarningForNode(uint8_t nodeID, uint32_t flags) {
    const uint32_t flagsMask = mask_u32(warningNodeFlagIndex);
    const uint32_t nodeBits = (uint32_t) (warningFrameFlagIndex - warningNodeFlagIndex);
    const uint32_t nodeIDMask = mask_u32(nodeBits);

    uint32_t data = 0u;

    // Flags (clip to allocated region)
    data |= (flags & flagsMask);
    // Node ID
    data |= (((uint32_t) (nodeID) &nodeIDMask) << warningNodeFlagIndex);
    // Doesnt set frameID or data (frameID = 0, dataNum = 0)

    // Compute payload length in bytes based on highest used bit.
    const uint32_t totalBits = warningDataFlagIndex + maxDataInFrameBits;
    const uint8_t payloadSize = (uint8_t) ((totalBits + 7u) >> 3);

    // send the warning
    CANPacket message = (CANPacket) {0};
    message.id = combinedID(warningCode, vitalsID);
    writeData(&message, (int8_t*) &data, payloadSize);
    sendPacket(&message);
}
