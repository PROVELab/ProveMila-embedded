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

struct CANPacket {
    int id;
    char data[8] = {0};
    char dataSize = 0;
};

// A single CANListenParam
struct CANListenParam {
    int listen_ids;
    int (*handler)(CANPacket *);
};

struct PCANListenParamsCollection{
    CANListenParam arr[1000];
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
int waitPacket(CANPacket * recv_pack, int listen_id, int (*handler)(CANPacket *));

int sendPacket(CANPacket*  p);

int getID(int fn_id, int node_id);
// Only in use with sensor stuff
void setSensorID(CANPacket * p, char sensorId);

// Write size bytes to the packet, accounting
// For Max Length
int writeData(CANPacket * p, char * dataPoint, int size);

struct Task{
    void (*function)(void); // Function to call
    unsigned int interval; // Milliseconds between task runs
    bool locked; // Lock CAN - Only applicable to multithreading
};

/* "Scheduler/TaskManager" */
class Schedule{
private:
    Task tasks[MAX_TASK_COUNT]; // A little mini-queue
    int ctr = 0; // Counts how many events are in queue
public:
    /* 
    Add the task to the task queue (will all be enabled
    when mainloop is called)
    return: PCAN_ERR - denotes if the we have too many tasks,
        or success
     */
    PCAN_ERR scheduleTask(Task t);
    // Loop through the tasks, enabling all of them with
    // their specifications listening for packets
    [[noreturn]] void mainloop();
};

#endif