#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"

enum PCAN_ERR{
    NOSPACE=-1,
    SUCCESS = 0,
};

struct CANPACKET{
    int id;
    byte data[8];
    byte dataSize = 0;
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
    void (**handler)(CANPACKET *);
};


/// @brief Blocking wait on a packet with listen_id, other packets are ignored
/// @param recv_pack a pointer to a packet-sized place in 
///                  such that the caller can see what
///                  the received packet is if necessary
///                  If NULL, the recv_pack will create its own
/// @param listen_id the integer id we're listening for
/// @param handler a function that takes in a
///                pointer to the received packet
/// @return 0 on success, nonzero on Failure (see PCAN_ERR enum)
int waitPacket(CANPACKET * recv_pack, int listen_id, void (*handler)(CANPACKET *));
int waitPackets(CANPACKET * recv_pack, CANLISTEN_PARAM * params); // Pass by Ref

void sendPacket(CANPACKET p);

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

/* "Scheduler/TaskManager" */
class Schedule{
private:
    Task tasks[20];
    int ctr = 0;
public:
    // Add the task to the task array
    void scheduleTask(Task t);
    // Loop through the tasks, enabling all of them and 
    // listening for packets
    void mainloop();
};

struct Task{
    void (*function)(void); // Function to call
    int interval; // Milliseconds between task runs
    int startdelay; // Seconds until starting
    bool locked; // Lock CAN - Only applicable to multithreading
};

#endif