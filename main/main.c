#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include <string.h>
#include "esp_timer.h"

#include "mutex_declarations.h" //sets uo static mutexes. To add another mutex, declare it in this file, and its .c file, and increment mutexCount
#include "pecan/pecan.h"    //helper code for CAN stuff
#include "programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"

const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;

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
//

//initialize space for each task
#define STACK_SIZE 20000    //for deciding stack size, could check this out later, or not: https://www.freertos.org/Why-FreeRTOS/FAQs/Memory-usage-boot-times-context#how-big-should-the-stack-be
StaticTask_t sendHB_Buffer;
StackType_t sendHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t recieveMSG_Buffer;
StackType_t recieveMSG_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkStatus_Buffer;
StackType_t checkStatus_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t alert_Buffer;
StackType_t alert_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkHB_Buffer;
StackType_t checkHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack

TimerHandle_t missingDataTimers [ totalNumFrames ];  //one of these timers going off trigers callback function for missing CAN Data Frane
StaticTimer_t xTimerBuffers[ totalNumFrames ];      //array for the buffers of these timers
//Making nodes of vitals data struct
void startBus(){  //should only be called on start up and AFTER Bus has finished recovery
    int err=twai_start();
    if(err!=ESP_OK){    //restarts program in the event that we can't start the Bus. This should never happen 
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("Failed to start or restart driver with code: %d. Rebooting\n",err);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
        esp_restart();
    } else{
        mutexPrint("Driver Started\n\n\n\n");
    }
}


void check_bus_status(void * pvParameters){ //should send all this to telem
    for(;;){
        twai_status_info_t status_info;
        esp_err_t err = twai_get_status_info(&status_info);  // Correct function and argument
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("status error code: %d\n",err);
            printf("TWAI Status Information:\n");
            printf("State: %d\n", status_info.state); // Assuming `twai_state_t` can be cast to int
            if(status_info.state==TWAI_STATE_BUS_OFF){
                printf("initiating recovery\n");
                int recover;
                if ((recover=twai_initiate_recovery())!=ESP_OK){
                    printf("invalid recovery attempting to reboot. This should never happen\n");    //this should only be called here when Bus is off state, and we never unistall the driver, so error should not be possible
                    esp_restart();
                }
                printf("Recovery attempt Complete code: %d\n\n\n\n\n\n\n",recover);
            }
            printf("Messages to TX: %lu\n", status_info.msgs_to_tx);
            printf("Messages to RX: %lu\n", status_info.msgs_to_rx);
            printf("TX Error Counter: %lu\n", status_info.tx_error_counter);
            printf("RX Error Counter: %lu\n", status_info.rx_error_counter);
            printf("TX Failed Count: %lu\n", status_info.tx_failed_count);
            printf("RX Missed Count: %lu\n", status_info.rx_missed_count);
            printf("RX Overrun Count: %lu\n", status_info.rx_overrun_count);
            printf("Arbitration Lost Count: %lu\n", status_info.arb_lost_count);
            printf("Bus Error Count: %lu\n", status_info.bus_error_count);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}


// Function to check and print TWAI alerts
void twai_monitor_alerts(void * pvParameters) { //should send all this to Telem with HB
    uint32_t alerts;

    while (1) {
        mutexPrint("monitering\n\n");
        //check if an alerta has been raised
        int err = twai_read_alerts(&alerts, pdMS_TO_TICKS(0));  //do not block waiting for alert
        if(err!=0){
            //mutexPrint("error reading alert/no alert to be had");
        }
        if (alerts & TWAI_ALERT_TX_IDLE) {
            mutexPrint("TWAI Alert: TX Idle\n");
        }
        if (alerts & TWAI_ALERT_TX_SUCCESS) {
            mutexPrint("TWAI Alert: TX Success\n");
        }
        if (alerts & TWAI_ALERT_TX_FAILED) {
            mutexPrint("TWAI Alert: TX FAILED\n");
                twai_status_info_t status_info;
                esp_err_t err = twai_get_status_info(&status_info);  // Correct function and argument
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("Alert: The Transmission failed.");
            xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
        }
        if (alerts & TWAI_ALERT_RX_DATA) {
            mutexPrint("TWAI Alert: RX Data Available\n");
        }
        if (alerts & TWAI_ALERT_BELOW_ERR_WARN) {
            mutexPrint("TWAI Alert: Error counter below warning\n");
        }
        if (alerts & TWAI_ALERT_ERR_ACTIVE) {
            mutexPrint("TWAI Alert: Error Active\n");
        }
        if (alerts & TWAI_ALERT_BUS_OFF) {
            mutexPrint("TWAI Alert: Bus Off\n");
        }
        if (alerts & TWAI_ALERT_ABOVE_ERR_WARN) {
            mutexPrint("TWAI Alert: Above Error Warning Limit\n");
        }
        if (alerts & TWAI_ALERT_BUS_RECOVERED) {
            mutexPrint("TWAI Alert: Bus Recovered, \n\nrestarting Bus\n\n\n\n");
            startBus();
        }
        if (alerts & TWAI_ALERT_ARB_LOST) {
            mutexPrint("TWAI Alert: Arbitration Lost\n");
        }
        if (alerts & TWAI_ALERT_BUS_ERROR) {
            mutexPrint("TWAI Alert: Bus Error\n");
        }

        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}




    void recieveMSG(){   //prints information about and contents of every recieved message
        struct CANPacket message; //will store any recieved message
        //an array for matching recieved Can Packet's ID's to their handling functions. MAX length set to 20 by default initialized to default values
        struct PCANListenParamsCollection plpc={ .arr={{0}}, .defaultHandler = defaultPacketRecv, .size = 0, };
        
        for(;;){
            waitPackets(&message, &plpc);
            taskYIELD();    //task runs constantly since no delay, but on lowest priority, so effectively runs in the background
        }
    }

    void sendMCUMSG( void * pvParameters )    {

        for( ;; ) {
            //sampple code that writes DEADFACE as data for vital's HB
            struct CANPacket message={{0}};
            int8_t Command=0x40;
            int8_t Obj_one=0x18;
            int8_t Obj_two=0x10;
            int8_t sub_index=0x01;
            int8_t zero[4]={0};
            writeData(&message,&Command,1);
            writeData(&message,&Obj_one,1);
            writeData(&message,&Obj_two,1);
            writeData(&message,&sub_index,1);
            writeData(&message,zero,4);

            message.id=0x600+1; //HBPing, vitalsID
            if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            int err;
            if((err=sendPacket(&message))){
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("failed to send HB with code %d. \n",err);
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }else{
                mutexPrint("\n\nsent HB\n\n!");
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
void app_main(){    
    //Initialize configuration structures using macro initializers
    //a hasty testing of each pin found each numbered PIN un the board (0-35) worked for TWAI for each number that appears on the board, except for pin 34, and 35, which I don't beleive are actually GPIO pins, since the datasheet says the MCU has only 34 pins, so having a pin 35 wouldnt make much sense.
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

    TaskHandle_t sendHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                     sendMCUMSG,       /* Function that implements the task. */
                     "HeartBeatSend",          /* Text name for the task. */
                     STACK_SIZE,      /* Number of indexes in the xStack array. */
                     ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                     tskIDLE_PRIORITY,/* Priority at which the task is created. */
                     sendHB_Stack,          /* Array to use as the task's stack. */
                     &sendHB_Buffer,   /* Variable to hold the task's data structure. */
                     tskNO_AFFINITY);  //assigns printHello to core 0


    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //recieves CAN Messages 
                      recieveMSG,       /* Function that implements the task. */
                      "msgRecieve",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      recieveMSG_Stack,          /* Array to use as the task's stack. */
                      &recieveMSG_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0

    

    // TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore(  //prints out bus status info
    //                   check_bus_status,       /* Function that implements the task. */
    //                   "checkCan",          /* Text name for the task. */
    //                   STACK_SIZE,      /* Number of indexes in the xStack array. */
    //                   ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
    //                   1,/* Priority at which the task is created. */
    //                   checkStatus_Stack,          /* Array to use as the task's stack. */
    //                   &checkStatus_Buffer,   /* Variable to hold the task's data structure. */
    //                   tskNO_AFFINITY);  //assigns printHello to core 0

    // TaskHandle_t checkAlerts = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
    //                   twai_monitor_alerts,       /* Function that implements the task. */
    //                   "checkalrt",          /* Text name for the task. */
    //                   STACK_SIZE,      /* Number of indexes in the xStack array. */
    //                   ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
    //                   2,/* Priority at which the task is created. */
    //                   alert_Stack,          /* Array to use as the task's stack. */
    //                   &alert_Buffer,   /* Variable to hold the task's data structure. */
    //                   tskNO_AFFINITY);  //assigns printHello to core 0
    
    //vTaskStartScheduler();      /do not write vTaskStartScheduler anywhere if using IDF FreeRTOS, the scheduler begins running on initialization, we cant toggle it, and program crashes if you attempt to.
}