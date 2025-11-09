#include "../mcu/tasks/EEPROM/EEPROM.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define IDEAL_BLOCK_SIZE 64
#define TARGET_PERIOD    1000000 // Ideally we log at 1s intervals
#define EEPROM_BYTES     32768

typedef struct {
    uint32_t timestamp_ms;
    float avg_calculated_current_a; // Calculated current in Amps
    float avg_measured_voltage_v;   // Measured Voltage
    float avg_motor_speed;          // Average Motor Speed, RPM
    float avg_pedal;                // Average pedal "value"
} log_block;                        // 1s worth of data

#define MAGIC     "MILA"
#define MAGIC_LEN 4

typedef struct {
    char magic[4];          // "MILA"
    uint32_t bytes_written; // Total bytes written to the log
    uint16_t _end_addr;     // End address (write pointer) of the ring buffer (absolute EEPROM addr)
} log_file_superblock;

_Static_assert(sizeof(log_file_superblock) <= IDEAL_BLOCK_SIZE, "Superblock must fit in IDEAL_BLOCK_SIZE");

static const char* TAG = "LOG_DUMP";

// Layout:
// [ 0 .. IDEAL_BLOCK_SIZE-1 ]        : superblock
// [ IDEAL_BLOCK_SIZE .. EEPROM_BYTES ): ring buffer of log_block
static inline uint32_t ring_start(void) { return IDEAL_BLOCK_SIZE; }
static inline uint32_t ring_bytes(void) { return EEPROM_BYTES - IDEAL_BLOCK_SIZE; }
static inline uint32_t block_size(void) { return (uint32_t) sizeof(log_block); }
static inline uint32_t ring_capacity_blocks(void) { return ring_bytes() / block_size(); }

// Snap an absolute EEPROM address to the ring range and to block boundaries.
static uint32_t clamp_end_addr(uint32_t end_addr_abs) {
    uint32_t start = ring_start();
    uint32_t end = start + ring_capacity_blocks() * block_size(); // last valid byte + 1
    if (end <= start) return start;                               // degenerate, but safe-guard
    // If out of bounds, wrap it into the ring range
    uint32_t span = end - start;
    uint32_t rel = (end_addr_abs >= start) ? (end_addr_abs - start) : (UINT32_MAX - start + 1 + end_addr_abs) % span;
    rel %= span;
    // Align to block boundary
    rel -= (rel % block_size());
    return start + rel;
}

// Reads, validates, and (if needed) repairs the superblock in EEPROM.
// Also normalizes _end_addr from bytes_written modulo ring capacity.
static void readSuper(log_file_superblock* super) {
    uint8_t buf[IDEAL_BLOCK_SIZE];
    eeprom_read(0, buf, sizeof(buf));

    log_file_superblock on_disk = {0};
    memcpy(&on_disk, buf, sizeof(on_disk));

    bool magic_ok = (memcmp(on_disk.magic, MAGIC, MAGIC_LEN) == 0);

    if (!magic_ok) {
        // Initialize a fresh superblock
        memset(super, 0, sizeof(*super));
        memcpy(super->magic, MAGIC, MAGIC_LEN);
        super->bytes_written = 0;
        super->_end_addr = ring_start();
        ESP_LOGW(TAG, "Superblock missing/invalid. Reinitialized.");
    } else {
        *super = on_disk;
        ESP_LOGI(TAG, "Superblock found. bytes_written=%" PRIu32 ", _end_addr=%" PRIu16, super->bytes_written,
                 super->_end_addr);
    }

    // Normalize _end_addr from bytes_written (authoritative) and clamp
    uint32_t blocks_written_total = (super->bytes_written / block_size());
    uint32_t cap_blocks = ring_capacity_blocks();
    uint32_t end_index = (cap_blocks == 0) ? 0 : (blocks_written_total % cap_blocks);
    uint32_t computed_end = ring_start() + end_index * block_size();
    uint32_t clamped_end = clamp_end_addr(computed_end);

    if (super->_end_addr != (uint16_t) clamped_end) {
        ESP_LOGI(TAG, "Fixing _end_addr: %u -> %u", (unsigned) super->_end_addr, (unsigned) clamped_end);
        super->_end_addr = (uint16_t) clamped_end;
    }

    // Write back the (possibly repaired) superblock
    memset(buf, 0, sizeof(buf));
    memcpy(buf, super, sizeof(*super));
    eeprom_write(0, buf, sizeof(buf));
}

// Pretty-print a single log block
static void print_log_block(const log_block* b, uint32_t index, uint32_t base_ms) {
    // If timestamp_ms is absolute, print directly. If it’s relative, base_ms can help.
    (void) base_ms;
    ESP_LOGI(TAG, "[%5" PRIu32 "] t=%" PRIu32 " ms | I=%.3f A | V=%.3f V | RPM=%.1f | pedal=%.3f", index,
             b->timestamp_ms, b->avg_calculated_current_a, b->avg_measured_voltage_v, b->avg_motor_speed, b->avg_pedal);
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500)); // wait for monitor to open
    eeprom_init_default();
    // 1) Read/repair superblock
    log_file_superblock sb = {0};
    readSuper(&sb);

    // 2) Derive ring geometry
    const uint32_t cap_blocks = ring_capacity_blocks();
    if (cap_blocks == 0) {
        ESP_LOGE(TAG, "Ring capacity is zero. Check EEPROM_BYTES and struct sizes.");
        return;
    }

    // 3) Figure out how many blocks have ever been written and how many are currently stored
    const uint32_t total_blocks_written = sb.bytes_written / block_size();
    const uint32_t live_blocks = (total_blocks_written < cap_blocks) ? total_blocks_written : cap_blocks;

    if (live_blocks == 0) {
        ESP_LOGW(TAG, "No log data to display.");
        return;
    }

    // 4) Compute indices:
    //    end_addr points to the *next* write position (i.e., just past the newest block).
    //    Convert to a ring index [0, cap_blocks).
    const uint32_t end_index = (sb._end_addr - ring_start()) / block_size();
    const uint32_t start_index = (end_index + cap_blocks - live_blocks) % cap_blocks;

    ESP_LOGI(TAG, "Dumping %" PRIu32 " blocks (capacity=%" PRIu32 ", start=%" PRIu32 ", end=%" PRIu32 ")", live_blocks,
             cap_blocks, start_index, end_index);

    // 5) Walk from oldest -> newest, read and print
    for (uint32_t i = 0; i < live_blocks; ++i) {
        uint32_t idx = (start_index + i) % cap_blocks;
        uint32_t addr = ring_start() + idx * IDEAL_BLOCK_SIZE;
        log_block b;
        eeprom_read(addr, (uint8_t*) &b, sizeof(b));
        print_log_block(&b, i, 0);
    }

    ESP_LOGI(TAG, "=== Log dump complete ===");
}
