#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include "mutex_declarations.h"

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"
int init=0;//indicates that mutexes have been initialized, so we may print a warning for allocations and frees
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps){
    if(init){
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("Warning, allocating memory!\n"); // Call the non-reentrant function safely.
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("alloc warning cant safely print, in deadlock!\n"); }
    }
}
void esp_heap_trace_free_hook(void* ptr){
    if(init){
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("Warning: freeing memory!\n"); // Call the non-reentrant function safely.
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("free warning cant safely print, in deadlock!\n"); }
    }
}
//

//initialize space for each task
#define STACK_SIZE 20000    //for deciding stack size, could check this out later, or not: https://www.freertos.org/Why-FreeRTOS/FAQs/Memory-usage-boot-times-context#how-big-should-the-stack-be
StaticTask_t printHello_TaskBuffer;
StackType_t printHello_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
    
    void printHello( void * pvParameters )    {
        configASSERT( ( uint32_t ) pvParameters == 1UL );   //we can pass parameters to this task! (we passed one)
        for( ;; )        {
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("hello\n"); // Call the non-reentrant function safely.
                xSemaphoreGive(*printfMutex); // Release the mutex.
            }else { printf("cant print, in deadlock!\n"); }

            //allocate memory, this should cause a warning!
            int* testDetection=malloc(sizeof(int));
            *testDetection=5;
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                printf("testDetection: %d\n",*testDetection);   //need to print allocated value so it doesn't get optimized out
                xSemaphoreGive(*printfMutex);
            }else { printf("cant print, in deadlock!\n"); }

            free(testDetection);
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }

void app_main(){    
    mutexInit();    //initialize mutexes
    init=1;
    TaskHandle_t helloHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                      printHello,       /* Function that implements the task. */
                      "printHello",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      printHello_Stack,          /* Array to use as the task's stack. */
                      &printHello_TaskBuffer,   /* Variable to hold the task's data structure. */
                      0);  //assigns printHello to core 0
    //vTaskStartScheduler();      /do not write vTaskStartScheduler anywhere if using IDF FreeRTOS, the scheduler begins running on initialization, we cant toggle it, and program crashes if you attempt to.
}