// USED for testing EEPROM code, you probs just want EEPROM.h and EEPROM.c!

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "EEPROM.h"

static const char* TAG = "EEP_TEST";

static void dump_hex(const char* title, const uint8_t* buf, size_t len)
{
    printf("---- %s (len=%u) ----\n", title, (unsigned)len);
    for (size_t i = 0; i < len; ++i) {
        if ((i % 16) == 0) printf("%04x: ", (unsigned)i);
        printf("%02x ", buf[i]);
        if ((i % 16) == 15) printf("\n");
    }
    if ((len % 16) != 0) printf("\n");
}

static void fill_pattern(uint8_t* buf, size_t len, uint8_t seed)
{
    for (size_t i = 0; i < len; ++i) buf[i] = (uint8_t)(seed + i);
}

static void check_equal(const char* what, const uint8_t* a, const uint8_t* b, size_t len)
{
    if (memcmp(a, b, len) == 0) {
        ESP_LOGI(TAG, "%s: PASS", what);
    } else {
        ESP_LOGE(TAG, "%s: FAIL (mismatch)", what);
        dump_hex("expected", a, len);
        dump_hex("actual  ", b, len);
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));   // wait for monitor to open

    ESP_ERROR_CHECK(eeprom_init(25, 26));

    esp_err_t err;

    // -------------------------
    // Test 1: write/read 1 byte
    // -------------------------
    {
        uint16_t addr = 0x0000;
        uint8_t w = 0xA5;
        uint8_t r = 0x00;

        ESP_LOGI(TAG, "T1: write 1 byte @0x%04X = 0x%02X", addr, w);
        ESP_ERROR_CHECK(eeprom_write(addr, &w, 1));
        ESP_ERROR_CHECK(eeprom_read(addr, &r, 1));
        ESP_LOGI(TAG, "T1: read  1 byte @0x%04X = 0x%02X", addr, r);

        if (w == r) ESP_LOGI(TAG, "T1: PASS");
        else        ESP_LOGE(TAG, "T1: FAIL");
    }

    // ----------------------------------------------
    // Test 2: write/read small block (17 bytes) in-page
    // ----------------------------------------------
    {
        // Choose an address safely within a page (offset 0x20 in the page):
        // 0x0020 -> offset 32, 32 + 17 = 49 <= 64, so no page crossing.
        uint16_t addr = 0x0020;
        uint8_t wbuf[17], rbuf[17];
        fill_pattern(wbuf, sizeof(wbuf), 0x10);

        ESP_LOGI(TAG, "T2: write %u bytes @0x%04X (in-page)", (unsigned)sizeof(wbuf), addr);
        ESP_ERROR_CHECK(eeprom_write(addr, wbuf, sizeof(wbuf)));
        ESP_ERROR_CHECK(eeprom_read(addr, rbuf, sizeof(rbuf)));

        check_equal("T2 compare", wbuf, rbuf, sizeof(wbuf));
    }

    // ---------------------------------------------
    // Test 3: write/read a full page (64 bytes) aligned
    // ---------------------------------------------
    {
        // Page-aligned address: 0x0100 (0x0100 % 64 == 0)
        uint16_t addr = 0x0100;
        uint8_t wbuf[64], rbuf[64];
        fill_pattern(wbuf, sizeof(wbuf), 0x40);

        ESP_LOGI(TAG, "T3: write %u bytes @0x%04X (full page)", (unsigned)sizeof(wbuf), addr);
        ESP_ERROR_CHECK(eeprom_write(addr, wbuf, sizeof(wbuf)));
        ESP_ERROR_CHECK(eeprom_read(addr, rbuf, sizeof(rbuf)));

        check_equal("T3 compare", wbuf, rbuf, sizeof(wbuf));
    }

    // ---------- Test 4: crossing page write (auto-split) ----------
    {
        // 0x0130 is offset 48 within page starting at 0x0100; 32 bytes crosses into next page.
        uint16_t addr = 0x0130;
        uint8_t wbuf[32], rbuf[32];
        fill_pattern(wbuf, sizeof(wbuf), 0x80);

        ESP_LOGI(TAG, "T4: crossing write auto-split @0x%04X len=%u", addr, (unsigned)sizeof(wbuf));
        esp_err_t err = eeprom_write(addr, wbuf, sizeof(wbuf));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "T4 write failed: %s", esp_err_to_name(err));
        } else {
            err = eeprom_read(addr, rbuf, sizeof(rbuf));
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "T4 read failed: %s", esp_err_to_name(err));
            } else {
                check_equal("T4 compare", wbuf, rbuf, sizeof(wbuf));
            }
        }
    }

    ESP_LOGI(TAG, "All tests finished.");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
