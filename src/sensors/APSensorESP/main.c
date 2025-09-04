#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "../../pecan/pecan.h"                  //used for CAN
#include "../common/sensorHelper.hpp"           //used for compliance with vitals and sending data
#include "myDefines.hpp"          //contains #define statements specific to this node like myId.
#include "../../espBase/debug_esp.h"
//add declerations to allocate space for additional tasks here as needed
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; //buffer that the task will use as its stack

//if no special behavior, all you need to fill in the collectData<NAME>() function(s). Have them return an int32_t with the corresponding data
int32_t collect_airPressure(){
    int32_t airPressure = 2147483647;
    mutexPrint("collecting airPressure\n");
    return airPressure;
}

void recieveMSG(){  //task handles recieving Messages
	CANPacket message; //will store any recieved message
	PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };        //an array for matching recieved Can Packet's ID's to their handling functions.
	vitalsInit(&plpc,NULL); //vitals Compliance

	//declare CanListenparams here, each param has 3 entries. When recieving a msg whose id matches 'listen_id' according to matchtype (or 'mt'), 'handler' is called.
	//see Vitals' recieveMSG function for an example of what this looks like

	//this task will the call the appropriate ListenParams function when a CAN message is recieved
	for(;;){
		waitPackets(&message, &plpc);
		taskYIELD();    //task runs constantly since no delay, but on lowest priority (idlePriority), so effectively runs in the background
	}
}

void app_main(void){
	pecanInit config={.nodeId= myId, .txPin= defaultPin, .rxPin= defaultPin};
	pecan_CanInit(config);   //initialize CAN

	//Declare tasks here as needed
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
