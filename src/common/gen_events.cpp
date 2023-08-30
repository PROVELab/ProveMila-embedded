#include "pecan.hpp"

PCAN_ERR Schedule::scheduleTask(Task t){
    if (ctr >= MAX_TASK_COUNT){
        return NOSPACE;
    }
    this->tasks[ctr] = t;
    ctr++;
    return SUCCESS;
}