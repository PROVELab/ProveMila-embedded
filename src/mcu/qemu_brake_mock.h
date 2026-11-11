#ifndef QEMU_BRAKE_MOCK_H
#define QEMU_BRAKE_MOCK_H

#include "../pecan/pecan.h"

void start_qemu_brake_mock(void);
int qemu_brake_register_listener(PCANListenParamsCollection* plpc, int nodeId);
#endif