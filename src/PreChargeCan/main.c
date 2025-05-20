#include "../pecan/pecan.h"
#include "driver/gpio.h"
#include "../espMutexes/mutex_declarations.h"
#include "programConstants.h"
#define CONTACTOR_1 7
#define CONTACTOR_2 8
#define CONTACTOR_3 9

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"

int init=0;//indicates that mutexes have been initialized, so we may print a warning for allocations and frees
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps){  //is called every time memory is allocated, feature enabled in menuconfig
    if(init){
        mutexPrint("Warning, allocating memory!\n");
    }
}
void esp_heap_trace_free_hook(void* ptr){
    if(init){
        mutexPrint("Warning, freeing memory!\n");
    }
}

//initialize space for each task
#define STACK_SIZE 20000    //for deciding stack size, could check this out later, or not: https://www.freertos.org/Why-FreeRTOS/FAQs/Memory-usage-boot-times-context#how-big-should-the-stack-be

StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack

//turn on all contactors
int16_t turnOn(CANPacket *message) {
    mutexPrint("Turning on contactors\n");
    gpio_set_level((gpio_num_t)CONTACTOR_1, 1);  //turn on contactor 1
    gpio_set_level((gpio_num_t)CONTACTOR_2, 1);  //turn on contactor 2
    gpio_set_level((gpio_num_t)CONTACTOR_3, 1);  //turn on contactor 3
    return 0;
}

//turn off all contactors
int16_t shutoff(CANPacket *message) {
    mutexPrint("Shutting off contactors\n");
    gpio_set_level((gpio_num_t)CONTACTOR_1, 0);  //turn off contactor 1
    gpio_set_level((gpio_num_t)CONTACTOR_2, 0);  //turn off contactor 2
    gpio_set_level((gpio_num_t)CONTACTOR_3, 0);  //turn off contactor 3
    return 0;
}

void recieveMSG(void* param){   //prints information about and contents of every recieved message
    CANPacket message; //will store any recieved message
    //an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default initialized to default values
    PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0};
    //declare parameters here, each param has 3 entries. When recieving a msg whose id matches 'listen_id' according to 'mt', 'handler' is called.
    CANListenParam shutOffParam;
    shutOffParam.handler= shutoff;
    shutOffParam.listen_id =combinedID(shutoffCode, vitalsID);   //setting vitals ID doesnt matter, just checking function
    shutOffParam.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc,shutOffParam)!= SUCCESS){ //adds the parameter
        mutexPrint("plpc no room");
        while(1);
    }

    CANListenParam turnOnParam;
    turnOnParam.handler= turnOn;
    turnOnParam.listen_id =combinedID(turnOnCode, vitalsID);   //setting vitals ID doesnt matter, just checking function
    turnOnParam.mt=MATCH_FUNCTION; //MATCH_EXACT to make id and function code require match. MATCH_ID for same 7 bits of node ID. MATCH_FUNCTION for same 4 bits of function code
    if (addParam(&plpc,turnOnParam)!= SUCCESS){ //adds the parameter
        mutexPrint("plpc no room");
        while(1);
    }

    //this task will the call the appropriate ListenParams function when a CAN message is recieved
    for(;;){
        waitPackets(&message, &plpc);
        taskYIELD();    //task runs constantly since no delay, but on lowest priority, so effectively runs in the background
    }
}

void app_main() {
    // Configure GPIOs
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << CONTACTOR_1) | (1ULL << CONTACTOR_2) | (1ULL << CONTACTOR_3);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

   TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages 
                      recieveMSG,       /* Function that implements the task. */
                      "msgRecieve",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      recieveMSG_Stack,          /* Array to use as the task's stack. */
                      &recieveMSG_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0
}


