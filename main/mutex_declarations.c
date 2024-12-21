#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "stdio.h"
#include "mutex_declarations.h"  

#define mutexCount 2
//declare all mutexes as an array, so that they may be created in a loop
SemaphoreHandle_t mutexes[mutexCount];  
StaticSemaphore_t mutexBuffers [mutexCount];    //note that the usr does not interact with the buffers directly

//declare each mutex pointer, which will be used in program, essentially just giving each element of mutexes a name
SemaphoreHandle_t* printfMutex = &(mutexes[0]);
SemaphoreHandle_t* anotherMutex = &(mutexes[1]);


void mutexInit(void){   //initializes all mutexes in mutxes array in a loop
    // Create the mutex statically using xSemaphoreCreateMutexStatic
    
    for(int i=0;i<mutexCount;i++){
        (mutexes)[i]= xSemaphoreCreateMutexStatic(&(mutexBuffers[i]));
        if (mutexes[i] == NULL)    {
            printf("Failed to create mutex %d\n",i);
            return;
        }
    }

}