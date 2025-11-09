#ifndef EEPROM_H_
#define EEPROM_H_

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

#define DEFAULT_EEPROM_SDA  25
#define DEFAULT_EEPROM_SCLK 26

// Performance note: writes across a page require two writes. the code will handle it np,
// but if you prevent your write from writing across 64 byte boundaries, performance will be better.

// Write or read within 5ms of a write will trigger a sleep to ensure the write finishes
// If write/read fails, it will try again one time. for writes, this will induce a 5ms sleep
// (so dont call from same thread where you need to "PWM" the motor!)

esp_err_t eeprom_init_default(); // will use default pins above
esp_err_t eeprom_init(int sda_pin, int scl_pin);

// can write/read for 1 < len <= 64
esp_err_t eeprom_write(uint16_t addr, const uint8_t* data, uint8_t len);
esp_err_t eeprom_read(uint16_t addr, uint8_t* data_out, uint8_t len);

#endif /* EEPROM_H_ */
