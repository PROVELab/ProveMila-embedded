#include "qemu_brake_mock.h"

#include "esp_log.h"
#include "vsr.h"

#ifdef QEMU_BUILD

// QEMU-only CAN function ID. The relay sends two bytes:
// data[0] = low byte, data[1] = high byte of raw brake mV.
#define QEMU_BRAKE_SET_FUNCTION_ID 0b1110
#define QEMU_BRAKE_SUPPLY_MV       5000
#define QEMU_BRAKE_MAX_MV          500

static volatile int32_t qemuBrakeRawMv = 0;

static int16_t recvQemuBrake(CANPacket* p) {
    if (p->dataSize < 2) {
        return 0;
    }

    int32_t rawMv = (int32_t)p->data[0] | ((int32_t)p->data[1] << 8);
    qemuBrakeRawMv = rawMv > QEMU_BRAKE_MAX_MV ? QEMU_BRAKE_MAX_MV : rawMv;

    // Write directly into the shared vehicle-status register.
    volatile vehicle_status_reg_s* vsr = &vehicle_status_register;
    ACQ_REL_VSRSEM_W(brake_pedal, {
        vsr->brake_pedal.brake_raw_1 = (float)qemuBrakeRawMv;
        vsr->brake_pedal.brake_position_pct = (float)qemuBrakeRawMv;
        vsr->brake_pedal.brake_supply_voltage = QEMU_BRAKE_SUPPLY_MV;
    });

    ESP_LOGI("qemu_brake",
             "Received relay brake frame: %ld mV; VSR updated",
             qemuBrakeRawMv);

    return 0;
}

int qemu_brake_register_listener(PCANListenParamsCollection* plpc, int nodeId) {
    CANListenParam qemuBrakeParam = {
        .listen_id = combinedID(QEMU_BRAKE_SET_FUNCTION_ID, nodeId),
        .handler = recvQemuBrake,
        .mt = MATCH_EXACT,
    };

    return addParam(plpc, qemuBrakeParam);
}

#else

int qemu_brake_register_listener(PCANListenParamsCollection* plpc, int nodeId) {
    (void)plpc;
    (void)nodeId;
    return 0;
}

#endif

void start_qemu_brake_mock(void) {
#ifdef QEMU_BUILD
    ESP_LOGI("qemu_brake", "Brake mock ready; waiting for relay frames");
#endif
}