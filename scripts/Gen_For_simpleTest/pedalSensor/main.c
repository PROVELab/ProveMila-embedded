#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "../../pecan/pecan.h"             //used for CAN
#include "../common/sensorHelper.hpp"      //used for compliance with vitals and sending data
#include "myDefines.hpp"       //contains #define statements specific to this node like myId.
#include "../../espBase/debug_esp.h"
//add declerations to allocate space for additional tasks here as needed
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; //buffer that the task will use as its stack

//For Standard behavior, fill in the collectData<NAME>() function(s).
//In the function, return an int32_t with the corresponding data
int32_t collect_pedalPowerReadingmV(){
    int32_t pedalPowerReadingmV = 6000;
	mutexPrint("collecting pedalPowerReadingmV\n");
    return pedalPowerReadingmV;
}

int32_t collect_pedalReadingOne(){
    int32_t pedalReadingOne = 30;
	mutexPrint("collecting pedalReadingOne\n");
    return pedalReadingOne;
}

int32_t collect_pedalReadingTwo(){
    int32_t pedalReadingTwo = 30;
	mutexPrint("collecting pedalReadingTwo\n");
    return pedalReadingTwo;
}

void recieveMSG(){  //task handles recieving Messages
	PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };
	sensorInit(&plpc,NULL); //vitals Compliance

	//declare CanListenparams here, each param has 3 entries:
	//When recv msg with id = 'listen_id' according to matchtype (or 'mt'), 'handler' is called.
	
//task calls the appropriate ListenParams function when a CAN message is recieved
	for(;;){
		while(waitPackets(&plpc) != NOT_RECEIVED);
		taskYIELD();
	}
}

void app_main(void){
	base_ESP_init();
	pecanInit config={.nodeId= myId, .pin1= defaultPin, .pin2= defaultPin};
	pecan_CanInit(config);   //initialize CAN

	//Declare tasks here as needed
	TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages 
		recieveMSG,       /* Function that implements the task. */
		"msgRecieve",          /* Text name for the task. */
		STACK_SIZE,      /* Number of indexes in the xStack array. */
		( void * ) 1,    /* Task Parameter. Must remain in scope or be constant!*/ 
		tskIDLE_PRIORITY,/* Priority at which the task is created. */
		recieveMSG_Stack,          /* Array to use as the task's stack. */
		&recieveMSG_Buffer,   /* Variable to hold the task's data structure. */
		tskNO_AFFINITY);  //assign to either core
}
