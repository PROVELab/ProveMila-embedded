#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../espBase/debug_esp.h"
#include "../pecan/pecan.h"
#include "../programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

TimerHandle_t missingDataTimers[totalNumFrames];  // one of these timers going off trigers callback function for missing
                                                  // CAN Data Frane
StaticTimer_t xTimerBuffers[totalNumFrames];      // array for the buffers of these timers
static void vTimerCallback(TimerHandle_t xTimer); // callback for CanFrame Timeouts

int16_t monitorData(
    CANPacket* message) { // for now just stores the data (printing the past 10 node-frame- data (past 10) on each line)
    int16_t nodeId = IDTovitalsIndex(message->id);
    if (nodeId == invalidVitalsIndex) {
        mutexPrint("recieved data from invalid nodeId, ignoring\n");
        return 1;
    }

    vitalsNode* node = &(nodes[nodeId]); // the node which sent the message

    uint32_t CanFrameNumber = getDataFrameId(message->id); // the Can frame index is stored in extension
    if (CanFrameNumber > node->numFrames) {
        mutexPrint("invalid dataFrame. Ignoring data\n");
        return 1;
    }
    char str[15]; //  print info about the data we recieved.
    sprintf(str, "%ld, %d", CanFrameNumber, nodeId);
    mutexPrint(str);
    CANFrame* frame = &(node->CANFrames[CanFrameNumber]); // the frame this data corresponds to
    // mark this data as collected.
    sprintf(str, "%d", frame->frameID);
    mutexPrint(str);
    mutexPrint("markingFrame\n");

    if (xTimerReset(missingDataTimers[frame->frameID], pdMS_TO_TICKS(10)) == pdFAIL) { // wait up to 10ms to reset timer
        mutexPrint("warning, unable to reset timer");
        // Send warningFrame:
        sendWarningForDataPoint(frame, 0, frameTimerSetFail);
    } else {
        if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
            printf("timer Reset for node: %d, frame: %ld\n", nodeId, CanFrameNumber);
            xSemaphoreGive(printfMutex); // Release the mutex.
        } else {
            printf("cant print, in deadlock!\n");
        }
    }
    mutexPrint("timersSet\n");

    // parse each data from frame
    int8_t bitIndex = 0; // which bit of CANFrame we are currently reading from (as we iterate through the data)
    for (int i = 0; i < (*frame).numData; i++) {
        dataPoint* dataInfo = &(((*frame).dataInfo)[i]);
        uint32_t temp = 0;
        copyDataToValue(&temp, message->data, bitIndex, dataInfo->bitLength);
        int32_t recvdata = ((int32_t) temp) + dataInfo->min;

        frame->data[i][frame->dataLocation] = recvdata; // update the data
        sprintf(str, "recD: %ld", recvdata);
        mutexPrint(str);
        // increment bitIndex
        bitIndex += dataInfo->bitLength;
    }
    // increment dataLocation, mark that we have recorded the data:
    frame->dataLocation++; // increment the dataIndex
    if (frame->dataLocation == 10) { frame->dataLocation = 0; }
    frame->consecutiveMisses = 0;
    mutexPrint("monitor complete\n");
    return 0;
}

void initializeDataTimers() { // initializes timeOuts for Data collection, as soon as this runs, we need data from every
                              // node to be sending their data to prevent them getting flagged, or Bus off if critical
    int32_t numInits = 0;
    mutexPrint("initializing Timers\n");
    for (int i = 0; i < numberOfNodes; i++) {
        for (int j = 0; j < nodes[i].numFrames; j++) {
            missingDataTimers[numInits] =
                xTimerCreateStatic(/* Just a text name, not used by the RTOS kernel. */
                                   "Timer",
                                   /* The timer period in ticks, must be greater than 0. */
                                   pdMS_TO_TICKS(nodes[i].CANFrames[j].dataTimeout),
                                   /* The timers will auto-reload themselves when they expire. */
                                   pdTRUE,
                                   /* pointer to the canFrame to identify which frame is missing */
                                   (void*) &(nodes[i].CANFrames[j]),
                                   vTimerCallback,            // the callback function (same for all timers)
                                   &(xTimerBuffers[numInits]) // buffer that holds timer info stuff
                );
            if (missingDataTimers[numInits] == NULL) {
                mutexPrint("Error creating timer, aborting\n");
                while (1);
            }
            numInits++;
        }
    }
    // start the timers:
    for (int i = 0; i < numInits; i++) {
        if (xTimerStart(missingDataTimers[i], pdMS_TO_TICKS(1000)) ==
            pdFAIL) { // no time crunch yet, but if this isnt starting we want to be notified, instead of it to running
                      // forever
            mutexPrint("warning, unable to start a timer");
            while (1);
        }
    }
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
        printf("numer of inits: %ld\n", numInits); // Call the non-reentrant function safely.
        xSemaphoreGive(printfMutex);               // Release the mutex.
    } else {
        printf("cant print, in deadlock!\n");
    }
}

static void vTimerCallback(TimerHandle_t xTimer) { // called when data is never recieved. Triggers extrapolation, and
                                                   // extrapolation warning, sent directly to telem
    CANFrame* missingFrame = (CANFrame*) pvTimerGetTimerID(xTimer);
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
        printf("missing Data frame number: %d from node %d. \n", missingFrame->frameID, missingFrame->nodeID);
        xSemaphoreGive(printfMutex); // Release the mutex.
    } else {
        printf("cant print, in deadlock!\n");
    }
    // TODO: Add code or fnct call here to trigger extrapolation
    // Vitals does not yet actually monitor data (I was hoping to get som1 else to do it for me, since its a fairly open
    // and closed function,
    //  but I might end up doing it anyway). Also it would be good to get other people to agree on the algorithm
    //  (whether it be mine, theres, or a mix of both)

    // Send warning for extrapolation
    sendWarningForDataPoint(missingFrame, 0, missingFrameFlag | nonCriticalWarning);
}
