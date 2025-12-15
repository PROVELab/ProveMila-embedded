
#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../espBase/debug_esp.h"
#include "pecan.h"

void flexiblePrint(const char* str) { mutexPrint(str); }

// State changing management task.
#define busStatus_TaskSize 4096
static StaticTask_t busStatus_Task;
static StackType_t busStatus_Stack[busStatus_TaskSize];

void checkBusStatus(void* pvParameters) {
    int32_t myNodeId = *((int32_t*) pvParameters);
    uint32_t alerts;
    esp_err_t alertStatus;
    for (;;) {
        alertStatus = twai_read_alerts(&alerts, portMAX_DELAY);
        // mutexPrint("reading alert\n");
        if (alertStatus == ESP_OK) {
            if (alerts & TWAI_ALERT_BUS_OFF) {
                // mutexPrint("initiating recovery\n");
                if (twai_initiate_recovery() != ESP_OK) {
                    // mutexPrint("invalid recovery attempting to reboot. This should never happen\n");
                    // esp_restart();
                }

            } else if (alerts & TWAI_ALERT_BUS_RECOVERED) {
                // After recovering, twai enters stopped state. Lets enter the start state
                int err = twai_start();
                if (err != ESP_OK) {
                    // char buffer[70];
                    // sprintf(buffer, "error restarting Can: %d. Attempting to reboot\n", err);
                    // mutexPrint(buffer);
                    // esp_restart();
                } else {
                    // mutexPrint("Can Driver Started\n\n");
                    // send update indicating Bus restarted
                    sendStatusUpdate(canRecoveryFlag, myNodeId);
                }
            }
            if (alerts & TWAI_ALERT_RX_FIFO_OVERRUN) {
                // Hardware RX FIFO overrun (frames were dropped)
                mutexPrint("TWAI: RX FIFO overrun detected — at least one frame was lost\n");
                sendStatusUpdate(canRXOverunFlag, myNodeId);
            }
        } else if (alertStatus != ESP_ERR_TIMEOUT) {
            mutexPrint("confused on what state we are in. Should never happen. rebooting\n");
            esp_restart();
        }
    }
}
#define defaultTxPin GPIO_NUM_33
#define defaultRxPin GPIO_NUM_32

// Initialize Can for esps, also give logic for starting and restarting bus based on alerts
void pecan_CanInit(pecanInit config) {
    // parse config options
    static int nodeId;
    nodeId = config.nodeId; // need nodeId to persist, since used as task param
    const int txPin = config.pin1 == defaultPin ? defaultTxPin : config.pin1;
    const int rxPin = config.pin2 == defaultPin ? defaultRxPin : config.pin2;

    // Initialize configuration structures using macro initializers
    // TWAI_MODE_NORMAL
    //  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NO_ACK); //TWAI_MODE_NORMAL
    //  for standard behavior
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL); // TWAI_MODE_NORMAL for standard behavior
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver in pecan_CanInit\n");
        exit(1);
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start TWAI driver in pecan_CanInit\n");
        exit(1);
    }

    if (twai_reconfigure_alerts(
            TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_RX_FIFO_OVERRUN, // Alerts we care about
            NULL // NULL since dont need old alerts
            ) != ESP_OK) {
        mutexPrint("couldn't configure alerts. Attempting Restart\n");
        esp_restart();
    }
    // Create static State task
    xTaskCreateStatic(checkBusStatus,           // task function
                      "CAN_STATE",              // name
                      busStatus_TaskSize,       // stack depth
                      (void*) &nodeId,          // pass NodeID
                      configMAX_PRIORITIES - 1, // highest priority!
                      busStatus_Stack,          // stack buffer
                      &busStatus_Task           // task control block
    );

    sendStatusUpdate(initFlag, nodeId);
    return;
}

bool (*matcher[3])(uint32_t, uint32_t) = {
    exact, matchID, matchFunction}; // used by waitPackets to match incomming packets based on match type

// Unlike pecan for Arduino, this is blocking! (everything on esp should be a seperate task, as part of the esp-idf
// design philosophy, but make sure not to have other stuff in the same task with this! Matches any recieved packets
// with their handler Not thread-safe (only call from one thread). The packet reference is overriden upon call. returns
// value of the matching function, or NOT_RECIEVED for no new messages
int16_t waitPackets(PCANListenParamsCollection* plpc) {
    twai_message_t twaiMSG;
    static CANPacket recv_pack;
    if ((twai_receive(&twaiMSG, portMAX_DELAY) ==
         ESP_OK)) { // blocking check for messages (RTOS will schedule something else while blocked)
        if ((recv_pack.extendedID = twaiMSG.extd) == true) {
            recv_pack.id = twaiMSG.identifier & 0x1FFFFFFF; // view first 29 bits of id
        } else {
            recv_pack.id = twaiMSG.identifier & 0x7FF; // view first 11 bits of id
        }

        memset(recv_pack.data, 0, 8); // re-initialize data to all 0.
        if (twaiMSG.rtr) {            // for rtr packets, no need to look at data or data-size
            recv_pack.rtr = 1;
            recv_pack.dataSize = 0;
        } else { // not an rtr packet, copy the data_length and size
            recv_pack.rtr = 0;
            recv_pack.dataSize = twaiMSG.data_length_code;
            memcpy(recv_pack.data, twaiMSG.data, recv_pack.dataSize);
        }

        CANListenParam clp;
        // Then match the packet id with our params; if none matches, use default handler
        for (int16_t i = 0; i < plpc->size; i++) {
            clp = plpc->arr[i];
            if (matcher[clp.mt](recv_pack.id, clp.listen_id)) { return clp.handler(&recv_pack); }
        }
        return plpc->defaultHandler(&recv_pack);
    }
    return NOT_RECEIVED;
}

void sendPacket(CANPacket* p) {
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        mutexPrint("Packet Too Big\n");
        return;
    }
    twai_message_t message = {
        // This is the struct used to create and send a CAN message. Generally speaking, only the last 3 fields should
        // ever change.
        .extd = (p->id) > 0b11111111111, // Standard vs extended format. makes message extended if id is big enough
        .rtr = p->rtr,                   // Data vs RTR frame.
        .ss = 0,                         // Whether the message is single shot (i.e., does not repeat on error)
        .self = 0,                       // Whether the message is a self reception request (loopback)
        .dlc_non_comp = 0, // DLC is less than 8  I beleive, for our purposes, this should always be 0, we want to be
                           // compliant with 8 byte data frames, and not confuse arduino guys
        .identifier = p->id,
        .data_length_code = p->dataSize};
    // arduino's will through out any messages with DLC = 0. Arbitrarily set = 1.
    if (message.rtr) {
        message.data_length_code = 1;
        p->dataSize = 0;
    }
    memcpy(message.data, p->data, p->dataSize); // copy data into msg
    esp_err_t err;
    int transmitAttemptCount = 0;
    do {
        err = twai_transmit(&message, pdMS_TO_TICKS(10));
        if (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(
                10)); // give 10ms to let other message send, bus recover, or whatever else is going wrong.
            // char buffer[70];
            // sprintf(buffer, "error sending Can: %d\n", err);
            // mutexPrint(buffer);
        }
        transmitAttemptCount += 1;
    } while (err != ESP_OK && transmitAttemptCount != 50);

    if (transmitAttemptCount == 50) {
        // mutexPrint("Unable to transmit msg for at least 1 second of time. attempting reboot\n");
        // esp_restart();
    }
    // mutexPrint("sent Packet\n");
    // in current implementation, will always return ESP_OK.
    return;
}
