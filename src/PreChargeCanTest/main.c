#include "../pecan/pecan.h"
#include "../espMutexes/mutex_declarations.h"


void sendTurnOn(){
    //send messages to test CAN precharge shutoff and on messages
    mutexPrint("Sending turn on message\n");
    CANPacket turnOnMessage={0};
    setRTR(&turnOnMessage);
    turnOnMessage.id=combinedID(turnOnCode,vitalsID); //vitalsID
    int err;
    while((err=sendPacket(&turnOnMessage))){
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("failed to send with code %d. \n",err);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
        vTaskDelay(10/portTICK_PERIOD_MS);  //try to send HB again in 10ms
    }
}

void sendShutoff(){
    //send messages to test CAN precharge shutoff and on messages
    mutexPrint("Sending shutoff message\n");
    CANPacket shutoffMessage={0};
    setRTR(&shutoffMessage);
    shutoffMessage.id=combinedID(shutoffCode,vitalsID); //vitalsID
    int err;
    while((err=sendPacket(&shutoffMessage))){
        if (xSemaphoreTake(*printfMutex, portMAX_DELAY)) {
            printf("failed to send with code %d. \n",err);
            xSemaphoreGive(*printfMutex); // Release the mutex.
        }else { printf("cant print, in deadlock!\n"); }
        vTaskDelay(10/portTICK_PERIOD_MS);  //try to send HB again in 10ms
    }
}

void app_main(){
    while(1){
        vTaskDelay(2000/portTICK_PERIOD_MS);
        sendTurnOn();
        vTaskDelay(8000/portTICK_PERIOD_MS);
        sendShutoff();
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }
}