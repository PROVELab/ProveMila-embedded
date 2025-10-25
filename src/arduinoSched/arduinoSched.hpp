#ifndef ARDUINO_SPEC_H
#define ARDUINO_SPEC_H
#include "../pecan/pecan.h"
// Nothing Arduno sepcific in the header, we can use this same header for other schedulers in the future.

// Optionally pass via platformio build flags if you have concerns about number of tasks (not enough space, or not
// enough tasks) DO NOT #define from seperate file and include this. Other src files (like sensorHelper) include this
// header, and would need the same value.
#ifndef TASK_COUNT_OVERRIDE
#define MAX_TASK_COUNT 5 // Default Max number of tasks for Arduino
// #else
// #define MAX_TASK_COUNT TASK_COUNT_OVERRIDE
#endif

/* "Scheduler/TaskManager" */
class PScheduler {
  private:
    int16_t taskSpaceRemaining = MAX_TASK_COUNT; // Counts how many events are in queue
  public:
    // Main functions that will suffice for most nodes
    // interval = interval in ms between running function
    int scheduleTask(void (*function)(void), int16_t interval);
    void execute(); // run the scheduler. Call this in a loop.
    //

    // Other stuff:
    int scheduleOneTimeTask(void (*function)(void),
                            int16_t delay); // use the returned int to reference this task in runOneTimeTask
    void runOneTimeTask(int task, int delay);
    void changeInterval(int task, int newInterval);
    void changeIterations(int task, int numtIterations);
    //

    PScheduler();
};
#endif
