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

#include "../powerSensor/powerSensor.h"
//add declerations to allocate space for additional tasks here as needed
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[STACK_SIZE]; //buffer that the task will use as its stack

int32_t collect_pedalReadingOne(){

	// … every 10 ms:
	int32_t vin_mV = -20;
	selfPowerStatus_t ADC_Status = collectSelfPowermV(&vin_mV);
	selfPowerStatusCheck(ADC_Status, myId);
	if (vin_mV >= 0) {
		char buffer[32];
		sprintf(buffer, "Vin: %ld mV\n", vin_mV);
		mutexPrint(buffer);
		// use vin_mV
	}
    return vin_mV;
}

int32_t collect_pedalReadingTwo(){
    int32_t pedalReadingTwo = 50;
    // mutexPrint("c pedalReadingTwo");
    return pedalReadingTwo;
}



int16_t defaultPacketRecv2(CANPacket* p) {return 1;}


void recieveMSG(){  //task handles recieving Messages
	PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv2, .size = 0, };
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

	//Initialize ADC for self power reading
	selfPowerConfig cfg = {
		.ADCPin  = 34,     // GPIO34 (ADC1_CH6)
		.ADCUnit = 1,      // ADC1
		.R1 = 114000,       // 114k (Vin → ADC)
		.R2 = 57000,        // 57k (ADC → GND)
	};

	selfPowerStatus_t ADC_status = initializeSelfPower(cfg);
	selfPowerStatusCheck(ADC_status, myId);

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

