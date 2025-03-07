#include "logging.h"

void setup() {
    initLogger(115200);
    debug("ESP32 Debug Log Active");
    info("ESP32 Info Log");
    warn("ESP32 Warning!");
    error("ESP32 Error!");
}

void loop() {
    debug("ESP32 Loop Running...");
    delay(2000);
}