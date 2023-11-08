#ifndef PECAN_H
#define PECAN_H
#include "./ptypes.hpp"
#include <stdint.h>

enum PCAN_ERR{
    PACKET_TOO_BIG=-3,
    NOT_RECEIVED=-2,
    NOSPACE=-1,
    SUCCESS = 0,
    GEN_FAILURE = 1,
};

struct CANPacket {
    int16_t id;
    int8_t data[MAX_SIZE_PACKET_DATA] = {0};
    int8_t dataSize = 0;
};

// A single CANListenParam - one id and one function call
struct CANListenParam {
    int16_t listen_ids;
    int16_t (*handler)(CANPacket *);
};

// Multiple CANListenParams from above ^
struct PCANListenParamsCollection{
    CANListenParam arr[MAX_PCAN_PARAMS];
    int16_t size = 0;
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
int16_t waitPacket(CANPacket * recv_pack, int16_t listen_id, int16_t (*handler)(CANPacket *));

// Sends a packet
int16_t sendPacket(CANPacket* p);

// Constructs a correct CAN ID for packet via function code and specific node id
int16_t getID(int16_t fn_id, int16_t node_id);
// Only in use with sensor stuff
void setSensorID(CANPacket * p, uint8_t sensorId);

// Write size bytes to the packet, accounting
// For Max Length
int16_t writeData(CANPacket * p, int8_t * dataPoint, int16_t size);

struct PTask{
    void (*function)(void); // Function to call
    int16_t delay = 0; // Milliseconds from start before a task runs
    int16_t interval; // Milliseconds between task runs
    bool locked; // Lock CAN - Only applicable to multithreading
};

/* "Scheduler/TaskManager" */
class PScheduler{
private:
    int16_t ctr = 0; // Counts how many events are in queue
    PTask tasks[MAX_TASK_COUNT];

public:
    PScheduler();
    /* Add the task to the task queue (will all be enabled
    when mainloop is called)
    return: PCAN_ERR - denotes if the we have too many tasks,
        or success
     */
    PCAN_ERR scheduleTask(PTask t);
    // Loop through the tasks, enabling all of them with
    // their specifications listening for packets
    [[noreturn]] void mainloop(int8_t * inp);
};

#endif