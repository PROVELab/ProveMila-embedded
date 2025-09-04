
#include "freertos/FreeRTOS.h"  
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"

#include <string.h> 
#include <stdint.h>

#include "pecan.h"
#include "../espBase/debug_esp.h"

// State changing management task.
#define busStatus_TaskSize 4096
static StaticTask_t busStatus_Task;
static StackType_t  busStatus_Stack[busStatus_TaskSize];

void checkBusStatus(void *pvParameters) {
    int32_t myNodeId = *((int32_t*)pvParameters);
    uint32_t alerts;
    esp_err_t alertStatus;
    for (;;) {
        alertStatus = twai_read_alerts(&alerts, portMAX_DELAY);
        mutexPrint("reading alert\n");
        if(alertStatus  == ESP_OK){
            if (alerts & TWAI_ALERT_BUS_OFF) {
                mutexPrint("initiating recovery\n");
                if (twai_initiate_recovery() != ESP_OK) {
                    mutexPrint("invalid recovery attempting to reboot. This should never happen\n");
                    esp_restart();
                }

            } else if (alerts & TWAI_ALERT_BUS_RECOVERED) {
                //After recovering, twai enters stopped state. Lets enter the start state
                int err = twai_start();
                if (err != ESP_OK) {
                    char buffer[70];
                    sprintf(buffer, "error restarting Can: %d. Attempting to reboot\n", err);
                    mutexPrint(buffer);
                    esp_restart();
                } else {
                    mutexPrint("Can Driver Started\n\n");
                    // send update indicating Bus restarted
                    int16_t serr = sendStatusUpdate(canRecoveryFlag, myNodeId);
                    if (serr) {
                        char buffer[50];
                        sprintf(buffer, "error sending CAN recovery Msg: %d\n", serr);
                        mutexPrint(buffer);
                    }
                }
            }
        } else if(alertStatus != ESP_ERR_TIMEOUT){
            mutexPrint("confused on what state we are in. Should never happen. rebooting\n");
            esp_restart();
        }
    }
}

void pecan_CanInit(pecanInit config){
    //parse config options
    static int nodeId;
    nodeId = config.nodeId; //need nodeId to persist, since used as task param
    const int defaultTxPin = GPIO_NUM_33;
    const int defaultRxPin = GPIO_NUM_32;
    config.txPin = config.txPin == defaultPin ? defaultTxPin : config.txPin;
    config.rxPin = config.rxPin == defaultPin ? defaultRxPin : config.rxPin;
    //

	//Initialize configuration structures using macro initializers
    //TWAI_MODE_NORMAL
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(config.txPin, config.rxPin, TWAI_MODE_NORMAL); //TWAI_MODE_NORMAL for standard behavior  
	twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
	twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	//Install TWAI driver
	if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
		printf("Driver installed\n");
	} else {
		printf("Failed to install driver in pecan_CanInit\n");
		exit(1);
	}

	//Start TWAI driver
	if (twai_start() == ESP_OK) {
		printf("Driver started\n");
	} else {
		printf("Failed to start TWAI driver in pecan_CanInit\n");
		exit(1);
	}

    ESP_ERROR_CHECK(twai_reconfigure_alerts(
        TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED,   // Alerts we care about
        NULL                                             // Don’t need to get old alerts
    ));

    // Create static State task
    xTaskCreateStatic(
        checkBusStatus,         // task function
        "CAN_STATE",             // name
        busStatus_TaskSize,   // stack depth
        (void *)&nodeId, // pass NodeID
        configMAX_PRIORITIES - 1,    // highest priority!
        busStatus_Stack,        // stack buffer
        &busStatus_Task          // task control block
    );
    int16_t err;
    if((err= sendStatusUpdate(initFlag, nodeId))){
        char buffer[50];
        sprintf(buffer, "error sending Init Flag: %d\n", err);  // Convert the int8_t to a string
        mutexPrint(buffer);  
    }
    return;
}

//Simplified Can functionality Functions: feel free to never use these, and just use them as sample code for ESP-CAN library:
int16_t defaultPacketRecv(CANPacket* p){
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
        printf("Default handler: id %ld\n with data:", p->id);
        for(int i=0;i<p->dataSize;i++){
            printf("% d",(p->data)[i]);
        }
        printf("\n");
        xSemaphoreGive(printfMutex); // Release the mutex.
    }else { printf("cant print, in deadlock!\n"); }
    return 0;
}

bool (*matcher[3])(uint32_t, uint32_t) = {exact, matchID, matchFunction};   //could alwys be moved back to pecan.h as an extern variable if its needed elsewhere? I am not sure why this was declared there in the first place

int16_t waitPackets(CANPacket *recv_pack, PCANListenParamsCollection *plpc) {
    if (recv_pack == NULL) { //if no  packet provided, exit early. 
        return 1;
    }

    CANListenParam clp;
    twai_message_t twaiMSG;
        if ((twai_receive(&twaiMSG, portMAX_DELAY) == ESP_OK)) {   //check for message without blocking (0 ms blocking)
        //construct CANPacket
        if (twaiMSG.extd) {
            recv_pack->id =twaiMSG.identifier & 0xFFFFFFF;   //view first 28 bits of id
        } else{
            recv_pack->id =twaiMSG.identifier & 0b1111111111;    //view first 11 bits of id
        }
        memset(recv_pack->data, 0, 8);  //re-initialize data to all 0.
        if(twaiMSG.rtr){    //for rtr packets, no need to look at data or data-size
            recv_pack->rtr=1;
            recv_pack->dataSize=0;
            memset(recv_pack->data, 0,8);
        }else{  //not an rtr packet, copy the data_length and size
            recv_pack->rtr=0;
            recv_pack->dataSize=twaiMSG.data_length_code;
            memcpy(recv_pack->data, twaiMSG.data,recv_pack->dataSize);
        }

        // Then match the packet id with our params; if none
        // match, use default handler
        for (int16_t i = 0; i < plpc->size; i++) {
            clp = plpc->arr[i];
            if (matcher[clp.mt](recv_pack->id, clp.listen_id)) {
                return clp.handler(recv_pack);
            }
        }
        return plpc->defaultHandler(recv_pack);
        
    }
    return NOT_RECEIVED;
}

void sendPacket(CANPacket *p) {
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        mutexPrint("Packet Too Big\n");
        return;
    }
 twai_message_t message = {  //This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should ever change.
        .extd= (p->id)>0b11111111111,              // Standard vs extended format. makes message extended if
        .rtr = p->rtr,               // Data vs RTR frame. 
        .ss = 0,                // Whether the message is single shot (i.e., does not repeat on error)
        .self = 0,              // Whether the message is a self reception request (loopback)
        .dlc_non_comp = 0,      // DLC is less than 8  I beleive, for our purposes, this should always be 0, we want to be compliant with 8 byte data frames, and not confuse arduino guys
        .identifier=p->id,  //id of vitals Heart Beat Ping
        .data_length_code = p->dataSize,
    };
    memcpy(message.data, p->data, p->dataSize); //copy data into msg
    esp_err_t err;
    int transmitAttemptCount = 0;
    do {
        err=twai_transmit(&message, pdMS_TO_TICKS(10));
        if(err != ESP_OK){  
            vTaskDelay(pdMS_TO_TICKS(10)); //give 10ms to let other message send, bus recover, or whatever else is going wrong.
            char buffer[70];
            sprintf(buffer, "error sending Can: %d\n", err);
            mutexPrint(buffer);
        }
        transmitAttemptCount += 1;
    } while(err != ESP_OK && transmitAttemptCount!=100);
    
    if(transmitAttemptCount == 100){
        mutexPrint("Unable to transmit msg for at least 1 second of time. attempting reboot\n");
        esp_restart();
    }
    mutexPrint("sent Packet\n");
    //in current implementation, will always return ESP_OK.
    return;
}

