#include "../vsr.h"
#include "EEPROM/EEPROM.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "tasks.h"
#include <string.h>

#define IDEAL_BLOCK_SIZE 32
#define TARGET_PERIOD    1000000 // Ideally we log at 1s intervals
#define EEPROM_BYTES     32768

// In need of SERIOUS refactoring lol
typedef struct {
    struct {
        uint32_t timestamp_ms;
        float avg_calculated_current_a; // Calculated current in Amps
        float avg_measured_voltage_v;   // Measured Voltage
        float avg_motor_speed;          // Average Motor Speed, RPM
        float avg_pedal;                // Average pedal "value"
    } log_block;                        // 1s worth of data

    struct {
        // Metadata we need to calculate the log block
        // for example, how much to divide by to get the average
        int current_cnt;
        int voltage_cnt;
        int motor_cnt;
        int pedal_cnt;
    } log_metadata;
} log_struct_s;

#define MAGIC "MILA"
typedef struct {
    char magic[4];          //"The magic"
    uint32_t bytes_written; // Total bytes written to the log
    uint16_t _end_addr;     // The end address of the "ringbuffer"
} log_file_superblock;

_Static_assert(sizeof(((log_struct_s*) 0)->log_block) <= IDEAL_BLOCK_SIZE, "log_block too big");
_Static_assert(EEPROM_BYTES % IDEAL_BLOCK_SIZE == 0, "EEPROM size not page-multiple");

// Call this guy every 5 log writes
void write_superblock(log_file_superblock* sb) {
    // TODO: Write SB (with updated _end)
    uint8_t buf[IDEAL_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, sb, sizeof(*sb));
    eeprom_write(0, buf, sizeof(buf));
}

void write_log_struct(log_file_superblock* sb, log_struct_s* log_struct) {
    // Write and reset log struct, update superblock's end
    // Average the relevant items
    log_struct->log_block.avg_calculated_current_a /=
        (log_struct->log_metadata.current_cnt > 0) ? log_struct->log_metadata.current_cnt : 1;
    log_struct->log_block.avg_measured_voltage_v /=
        (log_struct->log_metadata.voltage_cnt > 0) ? log_struct->log_metadata.voltage_cnt : 1;
    log_struct->log_block.avg_motor_speed /=
        (log_struct->log_metadata.motor_cnt > 0) ? log_struct->log_metadata.motor_cnt : 1;
    log_struct->log_block.avg_pedal /=
        (log_struct->log_metadata.pedal_cnt > 0) ? log_struct->log_metadata.pedal_cnt : 1;
    log_struct->log_block.timestamp_ms = (uint32_t) (esp_timer_get_time() / 1000); // in ms
    // TODO: write log_struct to next block
    uint8_t buf[IDEAL_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &log_struct->log_block, sizeof(log_struct->log_block));

    // Force write its ideal size to EEPROM
    static const char* TAG = "ROMLOG";

    if (eeprom_write(sb->_end_addr, buf, sizeof(buf)) != ESP_OK) {
        ESP_LOGE(TAG, "Error logging data");
    } else {
        ESP_LOGI(TAG, "Logged data");
    }

    // Zero out log_struct
    memset(log_struct, 0, sizeof(log_struct_s));

    // Increment end addr
    // If this is the last block, wrap around
    if (sb->_end_addr >= (EEPROM_BYTES - IDEAL_BLOCK_SIZE)) {
        sb->_end_addr = IDEAL_BLOCK_SIZE; // wrap around and start at chunk 1
    } else {
        // Simply increment
        sb->_end_addr += IDEAL_BLOCK_SIZE;
    }
    sb->bytes_written += sizeof(buf);
}

void setup_log(log_file_superblock* sb, log_struct_s* log_struct) {
    // Write log_file_superblock
    memset(sb, 0, sizeof(log_file_superblock));
    const char* magic_val = MAGIC;
    memcpy(sb->magic, magic_val, sizeof(sb->magic));
    sb->_end_addr = 1 * IDEAL_BLOCK_SIZE;
    sb->bytes_written = 0;

    write_superblock(sb);

    // Setup the log struct
    memset(log_struct, 0, sizeof(log_struct_s));
}

void collect_data(log_struct_s* log_struct, volatile vehicle_status_reg_s* vsr) {
    // Collect data from VSR and add to log_struct
    motor_mspeed_status_s motor_power;
    motor_hspeed_status_s motor_speed;
    pedal_s pedal_data;

    // Collect motor power
    ACQ_REL_VSRSEM_R(motor_power, { motor_power = vsr->motor_power; });
    log_struct->log_block.avg_calculated_current_a += motor_power.calculated_dc_current_a;
    log_struct->log_metadata.current_cnt += 1;

    log_struct->log_block.avg_measured_voltage_v += motor_power.measured_dc_voltage_v;
    log_struct->log_metadata.voltage_cnt += 1;

    // Collect motor speed
    ACQ_REL_VSRSEM_R(motor_speed, { motor_speed = vsr->motor_speed; });
    log_struct->log_block.avg_motor_speed += (float) motor_speed.motor_speed;
    log_struct->log_metadata.motor_cnt += 1;

    // Collect pedal data
    ACQ_REL_VSRSEM_R(pedal, { pedal_data = vsr->pedal; });
    log_struct->log_block.avg_pedal += pedal_data.pedal_position_pct;
    log_struct->log_metadata.pedal_cnt += 1;
}

void mcu_eeprom_vsr_log() {
    // Initialize and set frequency to 200 Hz
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xLogFrequency = pdMS_TO_TICKS(5); // 200 Hz

    volatile vehicle_status_reg_s* vsr = &vehicle_status_register; // easier to type

    if (eeprom_init_default() != ESP_OK) {
        // Uhhhh yeah you should figure this out and reflash
        while (1) ESP_LOGE(__func__, "Failed to initialize EEPROM for VSR logging");
        return;
    }

    // Setup the log "file"
    log_file_superblock sb;
    log_struct_s ls;
    setup_log(&sb, &ls);

    int64_t period_start = esp_timer_get_time(); // prime once before the loop
    int log_writes = 0;

    // infinite loop log
    while (1) {
        // just collect data
        collect_data(&ls, vsr);

        // Check if it's time to write to the logfile
        if (esp_timer_get_time() - period_start >= TARGET_PERIOD) {
            // Write (at 1s interval)
            write_log_struct(&sb, &ls);
            // On the 10th write, update the superblock as well
            if (++log_writes % 10 == 0) {
                write_superblock(&sb);
                log_writes = 0;
            }
            period_start = esp_timer_get_time();
        }

        // Wait to get to 200 Hz
        xTaskDelayUntil(&xLastWakeTime, xLogFrequency);
    }
}

void start_logging_task() {
    static StackType_t eeprom_log_stack[DEFAULT_STACK_SIZE];
    static StaticTask_t eeprom_log_buffer;

    // create the task
    xTaskCreateStaticPinnedToCore(mcu_eeprom_vsr_log, "mcu_eeprom_vsr_log", DEFAULT_STACK_SIZE, NULL, LOGGING_TASK_PRIO,
                                  eeprom_log_stack, &eeprom_log_buffer, 0);
}
