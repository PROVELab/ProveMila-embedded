#ifndef MUTEX_TASK_H
#define MUTEX_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Declare the function that initializes the mutex
void mutexInit(void);

// Declare all mutexes. These are initialized in mutex_declarations.c, and may be used by any other file that includes them.
extern SemaphoreHandle_t* printfMutex;
extern SemaphoreHandle_t* anotherMutex;
#endif /* MUTEX_TASK_H */