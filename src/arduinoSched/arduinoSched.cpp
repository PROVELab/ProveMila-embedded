#include "Arduino.h"
#include "avr/wdt.h"
#include <TaskScheduler.h>

#include "../arduinoSched/arduinoSched.hpp"
#include "../pecan/pecan.h"
#include "CAN.h"

#include <Arduino.h>

typedef Task ard_event;

// A small wrapper for arkhipenko's Task library.
// This was created so we could in theory use the same scheduling code across all frameworks.
// ^Would have been the case if we stuck with mbed, but esp-idf scheduling doesn't really map onto the same scheduling
// layout. Nevertheless, still useful to simplify scheduling, and avoid dynamic memory allocations (per PROVE coding
// principles)

Task dTasks[MAX_TASK_COUNT] = {}; // Container holds the arkhipenko library Tasks
Scheduler Arduino_ts;             // arkhipenko's scheduler

PScheduler::PScheduler() {};

// function is the callback. interval to execute the functoin is in ms
int PScheduler::scheduleTask(void (*function)(void), int16_t interval) {
    if (taskSpaceRemaining == 0) { return NOSPACE; }
    taskSpaceRemaining--;
    dTasks[taskSpaceRemaining].set(interval, TASK_FOREVER, function);
    Arduino_ts.addTask(dTasks[taskSpaceRemaining]);
    dTasks[taskSpaceRemaining].enableDelayed(interval);
    return taskSpaceRemaining;
}

void PScheduler::runOneTimeTask(int task, int timeDelay) {
    if (task < taskSpaceRemaining || task >= MAX_TASK_COUNT) {
        Serial.println("Cant run this task! It does not exist");
        return;
    }
    dTasks[task].disable();
    dTasks[task].setIterations(1);
    dTasks[task].enableDelayed(timeDelay);
}

// function is the callback, delay is in ms
int PScheduler::scheduleOneTimeTask(void (*function)(void), int16_t delay) {
    if (taskSpaceRemaining == 0) { return NOSPACE; }
    taskSpaceRemaining--;
    // Task newTask;
    dTasks[taskSpaceRemaining].set(delay, 1, function);
    Arduino_ts.addTask(dTasks[taskSpaceRemaining]);
    dTasks[taskSpaceRemaining].enableDelayed(delay);
    return taskSpaceRemaining;
}

void PScheduler::changeInterval(int task, int newInterval) { dTasks[task].setInterval(newInterval); }
void PScheduler::changeIterations(int task, int numtIterations) { dTasks[task].setIterations(numtIterations); }
// Generally: call this in your Arduino loop.
void PScheduler::execute() { Arduino_ts.execute(); }
