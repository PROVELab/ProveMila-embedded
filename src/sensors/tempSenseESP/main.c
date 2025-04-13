#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "../../pecan/pecan.h"                  //used for CAN
// #include "../../pecan/espBusRestart.h"	//included and used in vitalsInit (sensorHelper.cpp)
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //contains #define statements specific to this node like myId.
#include "../../vitalsNode/mutex_declarations.h"

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"
int init=0; //indicates that mutexes have been initialized, so we may print a warning for allocations and frees
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps) {  //is called every time memory is allocated, feature enabled in menuconfig
	if(init) {
		mutexPrint("Warning, allocating memory!\n");
	}
}
void esp_heap_trace_free_hook(void* ptr) {
	if(init) {
		mutexPrint("Warning, freeing memory!\n");
	}
}

//add declerations to allocate space for additional tasks here as needed
#define STACK_SIZE 20000
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; //buffer that the task will use as its stack

//

//if no special behavior, all you need to fill in the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data
int32_t collect_temperature1(){
    int32_t temperature1 = 1;
    mutexPrint("collecting temperature1\n");
    return temperature1;
}

int32_t collect_temperature2(){
    int32_t temperature2 = 2;
    mutexPrint("collecting temperature2\n");
    return temperature2;
}

int32_t collect_temperature3(){
    int32_t temperature3 = 3;
    mutexPrint("collecting temperature3\n");
    return temperature3;
}

int32_t collect_airPressure(){
    int32_t airPressure = -2147483648;
    mutexPrint("collecting airPressure\n");
    return airPressure;
}

void recieveMSG(){  //task handles recieving Messages
	CANPacket message; //will store any recieved message
	PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };        //an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default initialized to default values
	vitalsInit(&plpc,NULL); //vitals Compliance, and creates listen param for heartbeats

	//declare CanListenparams here, each param has 3 entries. When recieving a msg whose id matches 'listen_id' according to matchtype (or 'mt'), 'handler' is called.
	//see Vitals' recieveMSG function for an example of what this looks like

	//this task will the call the appropriate ListenParams function when a CAN message is recieved
	for(;;){
		waitPackets(&message, &plpc);
		taskYIELD();    //task runs constantly since no delay, but on lowest priority (idlePriority), so effectively runs in the background
	}
}

void app_main(void){
	//Initialize configuration structures using macro initializers
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_32, TWAI_MODE_NORMAL); //TWAI_MODE_NORMAL for standard behavior  
	twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
	twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	//Install TWAI driver
	if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
		printf("Driver installed\n");
	} else {
		printf("Failed to install driver\n");
		return;
	}

	//Start TWAI driver
	if (twai_start() == ESP_OK) {
		printf("Driver started\n");
	} else {
		printf("Failed to start driver\n");
		return;
	}

	mutexInit();    //initialize mutexes
	init=1;
	uint32_t alerts_to_enable = TWAI_ALERT_ALL;
	if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
		printf("Alerts reconfigured\n");
	} else {
		printf("Failed to reconfigure alerts");
	}

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
