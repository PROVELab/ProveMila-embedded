#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/semphr.h"
#include "mutex_declarations.h"
#include "can_functions.h"
//tracing functionality to give warning for free or alloc.
#include "esp_heap_caps.h"

const int vitalsID=0b0000010;
const int sendPing=0b0011;
const int sendPong=0b0100;
const int transmitData=0b0111;

int init=0;//indicates that mutexes have been initialized, so we may print a warning for allocations and frees
void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps){
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
StaticTask_t recieveHB_Buffer;
StackType_t recieveHB_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t checkStatus_Buffer;
StackType_t checkStatus_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack
StaticTask_t alert_Buffer;
StackType_t alert_Stack[ STACK_SIZE ]; //buffer that the task will use as its stack

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


void check_bus_status(void * pvParameters){
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
void twai_monitor_alerts(void * pvParameters) {

    uint32_t alerts;

    while (1) {
        mutexPrint("monitering\n\n");
        // Block until an alert is raised
        int err = twai_read_alerts(&alerts, pdMS_TO_TICKS(10));
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
    //
    void sendHB( void * pvParameters )    {
        //configASSERT( ( uint32_t ) pvParameters == 1UL );   //we can pass parameters to this task! although, this isn't really needed for this code, and likely wont ever be used since all tasks are static
        for( ;; )        {
            twai_message_t message = {  //This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should ever change.
                .rtr = 0,               // Data vs RTR frame.  We should avoid sending RTR frames since the other CAN libraries don't explicitly support it (just send a data message with no data instead)
                .extd = 0,              // Standard vs extended format
                .ss = 0,                // Whether the message is single shot (i.e., does not repeat on error)
                .self = 0,              // Whether the message is a self reception request (loopback)
                .dlc_non_comp = 0,      // DLC is less than 8  I beleive, for our purposes, this should always be 0, we want to be compliant with 8 byte data frames, and not confuse arduino guys
                .identifier=combinedID(sendPing,vitalsID),  //id of vitals Heart Beat Ping
                .data_length_code = 4,
                .data = {0},
            };

            //Queue message for transmission
            int ret;
            if ((ret=twai_transmit(&message, pdMS_TO_TICKS(10000))) == ESP_OK) {    //transmits message 
                mutexPrint("Message queued for transmission\n");
            } else {    //notify in event of failure to transmit message
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("Failed to queue message for transmission, error code: %d",ret); 
                    xSemaphoreGive(*printfMutex); // Release the mutex.
                }else { printf("cant print, in deadlock!\n"); }
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }
    void recieveMSG(){   //prints information about and contents of every recieved message
        for(;;){
            twai_message_t message;
            if (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {   //check for message without blocking (0 ms blocking)
                if (message.extd) {
                    mutexPrint("Message is in Extended Format\n");
                } else {
                    mutexPrint("Message is in Standard Format\n");
                }
                if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                    printf("ID is %ld\n", message.identifier); 
                    xSemaphoreGive(*printfMutex); 
                }else { printf("cant print, in deadlock!\n"); }
                if (!(message.rtr)) {
                    for (int i = 0; i < message.data_length_code; i++) {
                        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
                            printf("Data byte %d = %d\n", i, message.data[i]);  //print data of recieved message byte by byte
                        xSemaphoreGive(*printfMutex); // Release the mutex.
                        }else { printf("cant print, in deadlock!\n"); }
                    }
                }
            }
            taskYIELD();    //task runs constantly since no delay, but on lowest priority, so effectively runs in the background
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
                     sendHB,       /* Function that implements the task. */
                     "HeartBeatSend",          /* Text name for the task. */
                     STACK_SIZE,      /* Number of indexes in the xStack array. */
                     ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                     3,/* Priority at which the task is created. */
                     sendHB_Stack,          /* Array to use as the task's stack. */
                     &sendHB_Buffer,   /* Variable to hold the task's data structure. */
                     tskNO_AFFINITY);  //assigns printHello to core 0
    TaskHandle_t recieveHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                      recieveMSG,       /* Function that implements the task. */
                      "HeartBeatRecieve",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      tskIDLE_PRIORITY,/* Priority at which the task is created. */
                      recieveHB_Stack,          /* Array to use as the task's stack. */
                      &recieveHB_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0

    TaskHandle_t checkCanHandler = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                      check_bus_status,       /* Function that implements the task. */
                      "checkCan",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      1,/* Priority at which the task is created. */
                      checkStatus_Stack,          /* Array to use as the task's stack. */
                      &checkStatus_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0

    TaskHandle_t checkAlerts = xTaskCreateStaticPinnedToCore(  //schedules the task to run the printHello function, assigned to core 0
                      twai_monitor_alerts,       /* Function that implements the task. */
                      "checkalrt",          /* Text name for the task. */
                      STACK_SIZE,      /* Number of indexes in the xStack array. */
                      ( void * ) 1,    /* Parameter passed into the task. */    // should only use constants here. Global variables may be ok? cant be a stack variable.
                      2,/* Priority at which the task is created. */
                      alert_Stack,          /* Array to use as the task's stack. */
                      &alert_Buffer,   /* Variable to hold the task's data structure. */
                      tskNO_AFFINITY);  //assigns printHello to core 0

    //vTaskStartScheduler();      /do not write vTaskStartScheduler anywhere if using IDF FreeRTOS, the scheduler begins running on initialization, we cant toggle it, and program crashes if you attempt to.
}