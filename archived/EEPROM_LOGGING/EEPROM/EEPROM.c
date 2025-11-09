// src/EEPROM/EEPROM.c
#include "EEPROM.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <unistd.h> // usleep

#define TAG "I2C_MEM"

#ifndef CONFIG_I2C_ADDRESS
#define CONFIG_I2C_ADDRESS 0x50 // 24C256 with A2/A1/A0 = GND
#endif
#ifndef I2C_FREQUENCY
#define I2C_FREQUENCY 100000 // 100 kHz default (device supports up to 400 kHz)
#endif
#ifndef I2C_TICKS_TO_WAIT
#define I2C_TICKS_TO_WAIT 100
#endif

// -------------------------- Static driver state ---------------------------
typedef struct {
    i2c_port_t port;
    uint8_t addr; // 7-bit I2C address
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
    uint32_t max_addr; // last valid byte address
    int sda_pin;
    int scl_pin;
} eeprom_state_t;

static eeprom_state_t s_eeprom = {0};

// Write-cycle timing state (simple time-based guard for tWC)
static int64_t s_last_write_us = -1;  // -1 = none yet
static const int64_t s_tWC_us = 5000; // 5 ms (datasheet max write cycle for AT24C256)

// ------------------------------ Internals ---------------------------------
static inline uint32_t compute_max_address(void) {
    // AT24C256 = 256 Kbit = 32 KiB = 32768 bytes
    return 32768u - 1u;
}

static void wait_for_write_complete(void) {
    if (s_last_write_us < 0) return;
    int64_t now = esp_timer_get_time();
    int64_t remain = s_tWC_us - (now - s_last_write_us);
    if (remain > 0) usleep((useconds_t) remain);
}

static inline void mark_write_complete(void) { s_last_write_us = esp_timer_get_time(); }

// ------------------------------ Public API --------------------------------
esp_err_t eeprom_init(int sda_pin, int scl_pin) {
    s_eeprom.port = I2C_NUM_0;
    s_eeprom.addr = CONFIG_I2C_ADDRESS; // 7-bit
    s_eeprom.max_addr = compute_max_address();
    s_eeprom.sda_pin = sda_pin;
    s_eeprom.scl_pin = scl_pin;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = s_eeprom.port,
        .sda_io_num = s_eeprom.sda_pin,
        .scl_io_num = s_eeprom.scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, // fine for short runs; externals recommended
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_eeprom.bus), TAG, "i2c_new_master_bus failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = s_eeprom.addr,
        .scl_speed_hz = I2C_FREQUENCY,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_eeprom.bus, &dev_cfg, &s_eeprom.dev), TAG,
                        "i2c_master_bus_add_device failed");

    ESP_LOGI(TAG, "EEPROM 24C256 @0x%02X, SDA=%d SCL=%d, size=%lu bytes", s_eeprom.addr, s_eeprom.sda_pin,
             s_eeprom.scl_pin, (unsigned long) (s_eeprom.max_addr + 1));
    return ESP_OK;
}
esp_err_t eeprom_init_default() { return eeprom_init(DEFAULT_EEPROM_SDA, DEFAULT_EEPROM_SCLK); }

// Add this static helper near your other statics in EEPROM.c
static esp_err_t write_chunk_1_to_64(uint16_t addr, const uint8_t* data, uint8_t len) {
    // preconditions: data!=NULL, 1<=len<=64, (addr%64)+len <= 64, and end <= max_addr
    wait_for_write_complete();

    uint8_t out[2 + 64];
    out[0] = (uint8_t) (addr >> 8);
    out[1] = (uint8_t) (addr & 0xFF);
    memcpy(&out[2], data, len);

    // Try once; if it fails, wait a tWC window and retry once more.
    esp_err_t ret = i2c_master_transmit(s_eeprom.dev, out, 2 + len, I2C_TICKS_TO_WAIT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C tx failed (err=%s). Retrying after tWC...", esp_err_to_name(ret));
        wait_for_write_complete();
        ret = i2c_master_transmit(s_eeprom.dev, out, 2 + len, I2C_TICKS_TO_WAIT);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C tx failed again (err=%s). Giving up.", esp_err_to_name(ret));
            return ret;
        }
    }

    mark_write_complete();
    return ESP_OK;
}

// REPLACE your current eeprom_write(...) with this version
esp_err_t eeprom_write(uint16_t addr, const uint8_t* data, uint8_t len) {
    if (!data) {
        ESP_LOGE(TAG, "invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    if (len == 0 || len > 64) {
        ESP_LOGE(TAG, "invalid write size: %u (must be 1..64)", (unsigned) len);
        return ESP_ERR_INVALID_ARG;
    }
    if (addr > s_eeprom.max_addr) {
        ESP_LOGE(TAG, "addr out of range: 0x%04X", addr);
        return ESP_ERR_INVALID_SIZE;
    }
    if ((uint32_t) addr + len - 1 > s_eeprom.max_addr) {
        ESP_LOGE(TAG, "write exceeds end (addr=0x%04X len=%u)", addr, len);
        return ESP_ERR_INVALID_SIZE;
    }

    // If it fits within the current 64B page, do one chunk.
    uint16_t page_off = addr & 0x003F; // offset within page
    if (page_off + len <= 64) { return write_chunk_1_to_64(addr, data, len); }

    // Otherwise split exactly at the page boundary: first up to end-of-page, then the remainder.
    uint8_t first_len = (uint8_t) (64 - page_off); // 1..63
    uint8_t second_len = (uint8_t) (len - first_len);

    esp_err_t ret = write_chunk_1_to_64(addr, data, first_len);
    if (ret != ESP_OK) return ret;

    // Second chunk starts at next page; guaranteed to be <=64 and page-contained.
    return write_chunk_1_to_64((uint16_t) (addr + first_len), data + first_len, second_len);
}

esp_err_t eeprom_read(uint16_t addr, uint8_t* data_out, uint8_t len) {
    if (!data_out) return ESP_ERR_INVALID_ARG;
    if (len == 0 || len > 64) {
        ESP_LOGE(TAG, "invalid read size: %u (must be 1..64)", (unsigned) len);
        return ESP_ERR_INVALID_ARG;
    }
    if ((uint32_t) addr + len - 1 > s_eeprom.max_addr) {
        ESP_LOGE(TAG, "read would exceed memory end (addr=0x%04X len=%u)", addr, len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Avoid issuing a read while the device may still be completing a prior write
    wait_for_write_complete();

    uint8_t out[2] = {(uint8_t) (addr >> 8), (uint8_t) (addr & 0xFF)};
    esp_err_t ret = i2c_master_transmit_receive(s_eeprom.dev, out, sizeof(out), data_out, len, I2C_TICKS_TO_WAIT);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "I2C receive failed (err=%s).", esp_err_to_name(ret)); }
    return ret;
}
