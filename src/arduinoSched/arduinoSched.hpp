#ifndef ARDUINO_SPEC_H
#define ARDUINO_SPEC_H
#include "../pecan/pecan.h"

struct PTask {
    void (*function)(void); // Function to call
    int16_t delay = 0;      // Milliseconds from start before a task runs
    int16_t interval;       // Milliseconds between task runs, a negative interval
                            // indicates a task occruing one time, -interval ms after
                            // its declared
    int16_t location;       // Location of Task in task array, assigned upon task
                            // scheduling
    bool locked;            // Lock CAN - Only applicable to multithreading
};

/* "Scheduler/TaskManager" */
class PScheduler {
  private:
    int16_t ctr = 0; // Counts how many events are in queue
                     // PTask tasks[MAX_TASK_COUNT];    //delete wen decide not to use originally
                     // scheduling int16_t dCtr=0;

  public:
    int scheduleTask(PTask* t);

    // use the returned int to reference this task in runOneTimeTask
    int scheduleOneTimeTask(PTask* t);

    void runOneTimeTask(int task, int delay);

    void changeInterval(int task, int newInterval);

    void changeIterations(int task, int numtIterations); //-1 for set forever

    void mainloop(PCANListenParamsCollection* listens);

    PScheduler();
    /* Add the task to the task queue (will all be enabled
    when mainloop is called)
    return: PCAN_ERR - denotes if the we have too many tasks,
        or success
     */
    // Loop through the tasks, enabling all of them with
    // their specifications listening for packets
};
#endif
