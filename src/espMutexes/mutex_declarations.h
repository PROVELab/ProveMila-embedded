#ifndef MUTEX_TASK_H
#define MUTEX_TASK_H

#ifdef __cplusplus
extern "C" { // Ensures C linkage for all functions. This is needed since
             // arduino and common files are cpp, while esp Specific files are
             // c, and pecan.h has function declarations for both. This will
             // compile all functions with C linkage
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Declare the function that initializes the mutex
void mutexInit(void);
void mutexPrint(const char* str);

// Declare all mutexes. These are initialized in mutex_declarations.c, and may
// be used by any other file that includes them.
extern SemaphoreHandle_t* printfMutex;
extern SemaphoreHandle_t* oneAtATimeMutex;

#ifdef __cplusplus
} // End extern "C"
#endif

#endif /* MUTEX_TASK_H */
