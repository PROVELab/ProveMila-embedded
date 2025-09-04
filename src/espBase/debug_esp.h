#ifndef DEBUG_ESP
#define DEBUG_ESP
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "stdint.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stddef.h>

#define STACK_SIZE 10000	//for how big we need tasks to be, This value should be plenty

void base_ESP_init();
//mutexPrint
void mutexPrint(const char* str);
extern SemaphoreHandle_t printfMutex;

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps);
void esp_heap_trace_free_hook(void* ptr);

#endif