#include "logging.h"

inline void initLogger(long baudRate = 115200) {
    Serial.begin(baudRate);
    while (!Serial && !Serial.available()) {
      delay(10);  //wait for serial connection
    }
}

void setup() {
    initLogger(115200);
    debug("Debug Log Active!");
    info("Info Log Active!");
    warn("Warning Log Active!");
    error("Error Log Active!");
}

void loop() {
    debug("Debug loop...");
    info("Info loop...");
    warn("Warning loop...");
    error("Error loop...");
    delay(2000);
}