
#ifndef ESP_BUS_RESTART_H
#define ESP_BUS_RESTART_H

#ifdef __cplusplus
extern "C" { // Ensures C linkage for all functions. This is needed since
             // arduino and common files are cpp, while esp Specific files are
             // c, and pecan.h has function declarations for both. This will
             // compile all functions with C linkage
#endif

void check_bus_status(void* pvParameters);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // ESP_BUS_RESTART_H
