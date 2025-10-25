#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#include "../espBase/debug_esp.h" //for checking and restarting CAN bus
#include "../pecan/pecan.h"       //helper code for CAN stuff
#include "../programConstants.h"
#include "vitalsHelper/vitalsHelper.h"
#include "vitalsHelper/vitalsStaticDec.h"

static void printAllData(); // not for final use. for testing only

static void checkHB(void* pvParameters);
StaticTask_t checkHB_Buffer;
StackType_t checkHB_Stack[STACK_SIZE];

int64_t HBSendTime = 0;
void sendHB(void* pvParameters) {
    // creates the checkHB task
    TaskHandle_t processHBResp = xTaskCreateStaticPinnedToCore( // checksHB responses
        checkHB,                                                /* Function that implements the task. */
        "checkHeartBeatResponses",                              /* Text name for the task. */
        STACK_SIZE,                                             /* Number of indexes in the xStack array. */
        (void*) 1, /* Parameter passed into the task. */ // should only use constants here. Global variables may be ok?
                                                         // cant be a stack variable.
        tskIDLE_PRIORITY,                                /* Priority at which the task is created. */
        checkHB_Stack,                                   /* Array to use as the task's stack. */
        &checkHB_Buffer,                                 /* Variable to hold the task's data structure. */
        tskNO_AFFINITY);                                 // assigns printHello to core 0

    for (;;) {
        // Send HB
        CANPacket message = {0};
        setRTR(&message);
        message.id = combinedID(HBPing, vitalsID); // HBPing, vitalsID
        sendPacket(&message);

        HBSendTime = esp_timer_get_time();
        mutexPrint("\n\nsent HB\n\n!");
        vTaskResume(processHBResp); // run task to process HB responses
        printAllData();             // for debugging, just printing data periodically to view it.
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

int16_t recieveHeartbeat(CANPacket* message) { // mark the HB for given node as recieved, recording time to respond
    char printer[50];
    sprintf(printer, "recieved Pong from: %lu", (message->id) & 0x7F);
    mutexPrint(printer);
    int16_t nodeIndex = IDTovitalsIndex(message->id);
    if (nodeIndex == invalidVitalsIndex) {
        mutexPrint("recieved HB from invalid nodeId, ignoring\n");
        return 0; // invalid id
    }
    int64_t responseTime = esp_timer_get_time() - HBSendTime;
    uint16_t responseTime16 = responseTime < 0      ? 0
                              : responseTime > 1023 ? 1023 // want value to fit into 10 bits
                                                    : responseTime;
    VitalsFlagSet(nodeIndex, HBFlag);
    HBTimeSet(nodeIndex, responseTime16);
    return 0;
}

// helper for check HB, arrays most be size slowestNodeCount
static inline void updateSlowestIndex(uint32_t* slowestNodesArray, int16_t* worstTimesArray, int8_t nodeIndex,
                                      int16_t time) {
    for (int i = 0; i < slowestNodeCount; i++) {
        if (worstTimesArray[i] < time) {
            worstTimesArray[i] = time;
            slowestNodesArray[i] = nodeIndex;
            return;
        }
    }
}

// ChatGPT encouraged static asserts based on Constants. They cant hurt ig?
void assertHBDefines() {
// bro why is this a thing? cpp devs need to be stopped
#if defined(__cplusplus)
#define STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#else
#define STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif
//

// Bits that are fixed in the status frame header.
#define HB_HEADER_BITS (HBupdateTypeBits + HBStatusFrameBits)
#define HB_NODE_SLOTS  (64 - HB_HEADER_BITS)         // node flags per status frame
#define HB_MAX_FRAMES  (1ULL << (HBStatusFrameBits)) // how many distinct frame numbers

    // 1) Header must leave room for at least one node bit.
    STATIC_ASSERT(HB_HEADER_BITS < 64, "type+frame header must be < 64 bits");
    STATIC_ASSERT(HB_NODE_SLOTS > 0, "no room for node bits in a status frame");

    // 2) Capacity: we must be able to represent all nodes across all frames.
    // Avoid division: nodes <= (frames * slots_per_frame)
    STATIC_ASSERT((uint64_t) (numberOfNodes) <= (HB_MAX_FRAMES * (uint64_t) HB_NODE_SLOTS),
                  "not enough status-frame bits to encode all nodes");

    // 3) (Optional) Your statusFrameNo storage must be wide enough.
    STATIC_ASSERT(HBStatusFrameBits <= (sizeof(uint8_t) * 8),
                  "statusFrameNo is uint8_t; increase its width or reduce HBStatusFrameBits");

    // 4) (Optional) Timing-frame packing fits in 64 bits.
    STATIC_ASSERT(HBupdateTypeBits + HBTimerMSBits + slowestNodeCount * (nodeIDSizeBits + HBTimerMSBits) <= 64,
                  "timing frame does not fit in 64 bits");
}

// two different frames for HB are sent. A timing frame and status frame(s) check diagram (should be in a memo at some
// point for details, of format, or look at the code
static void checkHB(void* pvParameters) {

    assertHBDefines();

    for (;;) {

        vTaskDelay(250 / portTICK_PERIOD_MS); // give nodes 250ms to respond
        mutexPrint("\n\nchecking HB\n\n\n");

        // Trackers
        int32_t totalResponseCount = 0;
        int32_t totalTimeMs = 0;
        uint32_t slowestNodeIndices[3] = {0};
        int16_t worstTimes[3] = {0};

        // For writing data
        const uint64_t startingMask = 1ULL << (HBupdateTypeBits + HBStatusFrameBits);
        uint64_t HBStatusWriteMask = startingMask;
        uint8_t statusFrameNo = 0;
        uint64_t data = HBupdateStatus | (statusFrameNo << HBupdateTypeBits);

        // Create Status Frame
        for (int i = 0; i < numberOfNodes; i++) {
            if (!HBStatusWriteMask) { // we have filled up this frame, lets send it!
                // sendFrame
                CANPacket message = {0};
                message.id = combinedID(HBRespUpdate, vitalsID);
                writeData(
                    &message, (int8_t*) &data,
                    sizeof(
                        data)); // relies on esp32 being little endian (since interpretting uint64_t as array of bytes)
                sendPacket(&message);

                // reset data for next frame
                statusFrameNo++;
                data = HBupdateStatus | (statusFrameNo << HBupdateTypeBits);
                HBStatusWriteMask = startingMask;
            }

            if (VitalsFlagsGet(i) &
                HBFlag) { // this is only place where HB flag gets cleared, so ok to check flag, then get time.
                data |= HBStatusWriteMask;  // indicate we recv this HB
                VitalsFlagClear(i, HBFlag); // reset HB bit
                int16_t responseTime = HBTimeGet(i);
                totalTimeMs += responseTime;
                updateSlowestIndex(slowestNodeIndices, worstTimes, i, responseTime);
                totalResponseCount++;
            }
            HBStatusWriteMask <<= 1; // shift the bit we write to to the next bit
        }
        // send last Status Frame:
        if (HBStatusWriteMask != startingMask) { // send the last frame (if we carried into it)
            CANPacket message = {0};
            message.id = combinedID(HBRespUpdate, vitalsID);
            writeData(
                &message, (int8_t*) &data,
                sizeof(data)); // relies on esp32 being little endian (since interpretting uint64_t as array of bytes)
            sendPacket(&message);
        }

        // create timing frame;
        int32_t averageTime = 0;
        if (totalResponseCount != 0) { // dont divide by 0
            averageTime = squeeze(totalTimeMs / totalResponseCount, 0, 1023);
        }
        uint8_t dataIndex = HBupdateTypeBits;
        data = HBupdateTiming | ((uint64_t) averageTime << dataIndex);
        dataIndex += HBTimerMSBits;

        for (int i = 0; i < slowestNodeCount && i < totalResponseCount; i++) {
            const uint16_t slowNodeID = vitalsIndexToID(slowestNodeIndices[i]) & ((1u << nodeIDSizeBits) - 1u);
            const uint16_t slowMs = (uint16_t) squeeze((int32_t) worstTimes[i], 0, (1 << HBTimerMSBits) - 1);

            data |= ((uint64_t) slowNodeID << dataIndex);
            dataIndex += nodeIDSizeBits;
            data |= ((uint64_t) slowMs << dataIndex);
            dataIndex += HBTimerMSBits;
        }

        // send timing frame:
        CANPacket timingMessage = {0};
        timingMessage.id = combinedID(HBRespUpdate, vitalsID);
        writeData(&timingMessage, (int8_t*) &data,
                  sizeof(data)); // relies on esp32 being little endian (since interpretting uint64_t as array of bytes)
        sendPacket(&timingMessage);

        vTaskSuspend(NULL);
    }
}

static void printAllData() { // not for final use. for testing only
    if (xSemaphoreTake(printfMutex, portMAX_DELAY)) {
        for (int i = 0; i < numberOfNodes; i++) { // each node
            printf("printData: node (vitalsId): %d, numFrams: %d\n", i, (nodes[i]).numFrames);
            for (int8_t j = 0; j < nodes[i].numFrames; j++) { // each frame
                printf("frameInfo: id: %d  numData: %d\n", j, ((nodes[i]).CANFrames[j]).numData);
                if ((nodes[i]).CANFrames == NULL) {
                    printf("error printg, framesptr not initialized, terminating\n");
                    return;
                }
                for (int8_t k = 0; k < (((nodes[i]).CANFrames)[j]).numData; k++) { // each data
                    printf("node: %d. frame: %d. datanum: %d data: ", i, j, k);
                    for (int l = 0; l < pointsPerData; l++) { printf("%ld ", nodes[i].CANFrames[j].data[k][l]); }
                    printf("\n");
                }
            }
        }
        xSemaphoreGive(printfMutex); // Release the mutex.
    } else {
        printf("cant print, in deadlock!\n");
    }
}
