
#include "driver/gpio.h"
#include "driver/twai.h"
#include "driver/uart.h"
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

#ifndef QEMU_BUILD
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

#else // QEMU_BUILD
// QEMU's ESP32 has no TWAI controller: the driver calls succeed but frames never leave the chip. In the QEMU build
// CAN frames are carried over UART1 instead. A host relay (emulation-tools/can_relay.py) joins the UART1 of
// every emulated node into one shared bus.
// Frame (16 bytes): [0]=0xA5 sync, [1]=flags (bit0 extended, bit1 rtr), [2..5]=id (little endian), [6]=dlc,
// [7..14]=data, [15]=XOR of bytes 1..14
#define QEMU_CAN_UART      UART_NUM_1
#define QEMU_CAN_TX_PIN    25
#define QEMU_CAN_RX_PIN    26
#define QEMU_CAN_SYNC      0xA5
#define QEMU_CAN_FRAME_LEN 16

static uint8_t qemuChecksum(const uint8_t* frame) {
    uint8_t sum = 0;
    for (int i = 1; i < QEMU_CAN_FRAME_LEN - 1; i++) { sum ^= frame[i]; }
    return sum;
}

void pecan_CanInit(pecanInit config) {
    uart_config_t uartConfig = {.baud_rate = 115200,
                                .data_bits = UART_DATA_8_BITS,
                                .parity = UART_PARITY_DISABLE,
                                .stop_bits = UART_STOP_BITS_1,
                                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                .source_clk = UART_SCLK_DEFAULT};
    if (uart_driver_install(QEMU_CAN_UART, 1024, 1024, 0, NULL, 0) != ESP_OK ||
        uart_param_config(QEMU_CAN_UART, &uartConfig) != ESP_OK ||
        uart_set_pin(QEMU_CAN_UART, QEMU_CAN_TX_PIN, QEMU_CAN_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) !=
            ESP_OK) {
        printf("Failed to set up CAN over UART1 in pecan_CanInit\n");
        exit(1);
    }
    printf("CAN over UART1 (QEMU) started\n");
    sendStatusUpdate(initFlag, config.nodeId);
}

// Blocks until a valid frame arrives. Bytes may arrive split, and a bad checksum means we lost sync, so keep scanning.
static bool qemuReadFrame(CANPacket* p) {
    uint8_t frame[QEMU_CAN_FRAME_LEN];
    for (;;) {
        if (uart_read_bytes(QEMU_CAN_UART, frame, 1, portMAX_DELAY) != 1 || frame[0] != QEMU_CAN_SYNC) { continue; }
        int got = 0;
        while (got < QEMU_CAN_FRAME_LEN - 1) {
            int n = uart_read_bytes(QEMU_CAN_UART, frame + 1 + got, QEMU_CAN_FRAME_LEN - 1 - got, pdMS_TO_TICKS(100));
            if (n <= 0) { break; }
            got += n;
        }
        if (got != QEMU_CAN_FRAME_LEN - 1 || qemuChecksum(frame) != frame[QEMU_CAN_FRAME_LEN - 1] ||
            frame[6] > MAX_SIZE_PACKET_DATA) {
            continue; // partial or corrupt frame, resync on the next sync byte
        }
        p->extendedID = frame[1] & 0x1;
        p->rtr = (frame[1] >> 1) & 0x1;
        p->id = (int32_t) ((uint32_t) frame[2] | (uint32_t) frame[3] << 8 | (uint32_t) frame[4] << 16 |
                           (uint32_t) frame[5] << 24);
        p->dataSize = p->rtr ? 0 : frame[6];
        memset(p->data, 0, MAX_SIZE_PACKET_DATA);
        memcpy(p->data, frame + 7, p->dataSize);
        return true;
    }
}

static void qemuSendFrame(const CANPacket* p) {
    uint8_t frame[QEMU_CAN_FRAME_LEN] = {0};
    const bool extended = p->id > 0b11111111111;
    frame[0] = QEMU_CAN_SYNC;
    frame[1] = (extended ? 0x1 : 0x0) | (p->rtr ? 0x2 : 0x0);
    frame[2] = p->id & 0xFF;
    frame[3] = (p->id >> 8) & 0xFF;
    frame[4] = (p->id >> 16) & 0xFF;
    frame[5] = (p->id >> 24) & 0xFF;
    frame[6] = p->rtr ? 0 : p->dataSize;
    memcpy(frame + 7, p->data, frame[6]);
    frame[QEMU_CAN_FRAME_LEN - 1] = qemuChecksum(frame);
    uart_write_bytes(QEMU_CAN_UART, frame, QEMU_CAN_FRAME_LEN); // one call per frame, so frames never interleave
}

// The motor task (src/mcu/motor_h300/motor_send.c, a git submodule) calls twai_transmit directly instead of going
// through sendPacket. src/CMakeLists.txt links QEMU builds with -Wl,--wrap=twai_transmit, so those calls land here and
// are sent over UART1 as well, without editing the submodule.
esp_err_t __wrap_twai_transmit(const twai_message_t* message, TickType_t ticks_to_wait) {
    (void) ticks_to_wait;
    CANPacket p = {0};
    p.id = message->identifier;
    p.extendedID = message->extd;
    p.rtr = message->rtr;
    p.dataSize = message->rtr ? 0
                              : (message->data_length_code > MAX_SIZE_PACKET_DATA ? MAX_SIZE_PACKET_DATA
                                                                                  : message->data_length_code);
    memcpy(p.data, message->data, p.dataSize);
    qemuSendFrame(&p);
    return ESP_OK;
}
#endif // QEMU_BUILD

bool (*matcher[3])(uint32_t, uint32_t) = {
    exact, matchID, matchFunction}; // used by waitPackets to match incomming packets based on match type

// Unlike pecan for Arduino, this is blocking! (everything on esp should be a seperate task, as part of the esp-idf
// design philosophy, but make sure not to have other stuff in the same task with this! Matches any recieved packets
// with their handler Not thread-safe (only call from one thread). The packet reference is overriden upon call. returns
// value of the matching function, or NOT_RECIEVED for no new messages
static int16_t dispatchPacket(PCANListenParamsCollection* plpc, CANPacket* recv_pack) {
    CANListenParam clp;
    // Match the packet id with our params; if none matches, use default handler
    for (int16_t i = 0; i < plpc->size; i++) {
        clp = plpc->arr[i];
        if (matcher[clp.mt](recv_pack->id, clp.listen_id)) { return clp.handler(recv_pack); }
    }
    return plpc->defaultHandler(recv_pack);
}

int16_t waitPackets(PCANListenParamsCollection* plpc) {
    static CANPacket recv_pack;
#ifdef QEMU_BUILD
    if (qemuReadFrame(&recv_pack)) { return dispatchPacket(plpc, &recv_pack); }
    return NOT_RECEIVED;
#else
    twai_message_t twaiMSG;
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

        return dispatchPacket(plpc, &recv_pack);
    }
    return NOT_RECEIVED;
#endif
}

void sendPacket(CANPacket* p) {
    if (p->dataSize > MAX_SIZE_PACKET_DATA) {
        mutexPrint("Packet Too Big\n");
        return;
    }
#ifdef QEMU_BUILD
    qemuSendFrame(p);
    return;
#endif
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
