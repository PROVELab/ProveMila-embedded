#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../vsr.h"
#include "tasks.h" // defines LOGGING_TASK_PRIO (fallback provided below if missing)

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#ifndef LOGGING_TASK_PRIO
#define LOGGING_TASK_PRIO 5
#endif

#include "diskio_impl.h" // for ff_diskio_register_sdmmc (ESP-IDF 5.3.x)
#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <unistd.h>

static const char* TAG = "SDFMT";
static sdmmc_card_t* g_card; // must live for the life of the mount

esp_err_t sd_mount(const char* base_path, bool allow_format_if_needed) {
    esp_vfs_fat_sdmmc_mount_config_t mc = {.format_if_mount_failed =
                                               allow_format_if_needed, // true => auto-FAT32 if needed
                                           .max_files = 8,
                                           .allocation_unit_size = 16 * 1024};

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING; // 400 kHz for bring-up

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1; // 1-bit: CLK14, CMD15, D0=2
    slot.gpio_cd = GPIO_NUM_NC;
    slot.gpio_wp = GPIO_NUM_NC;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    g_card = NULL; // mount will set this
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(base_path, &host, &slot, &mc, &g_card);
    if (ret != ESP_OK) {
        ESP_LOGE("SD", "mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdmmc_card_print_info(stdout, g_card); // expects sdmmc_card_t*
    ESP_LOGI("SD", "mounted at %s", base_path);
    return ESP_OK;
}

esp_err_t sd_unmount(const char* base_path) {
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(base_path, g_card);
    g_card = NULL;
    return ret;
}

static inline void log_fsync(FILE* fp) {
    fflush(fp);
    fsync(fileno(fp)); // forces FAT directory entry (size/time) to be updated
}

typedef struct {
    FILE* fp;
    char name[48];
} log_file_t;

static bool file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static void csv_header(FILE* fp);

/* Create first missing /sdcard/dataLogX.csv and write header */
static esp_err_t log_create_next(log_file_t* out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    out->fp = NULL;
    out->name[0] = '\0';

    for (int i = 0; i < 100000; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/sdcard/dataLog%d.csv", i);
        if (!file_exists(path)) {
            FILE* fp = fopen(path, "w");
            if (!fp) {
                ESP_LOGE(TAG, "Failed to create %s", path);
                return ESP_FAIL;
            }
            // print all fields of
            csv_header(fp);
            log_fsync(fp);
            setvbuf(fp, NULL, _IOLBF, 0); // line-buffered
            out->fp = fp;
            strncpy(out->name, path, sizeof(out->name) - 1);
            out->name[sizeof(out->name) - 1] = '\0';
            ESP_LOGI(TAG, "Created %s", out->name);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

// make sure these two are consistant with VSR
static void csv_header(FILE* fp) {
    // motor_mspeed_status_s
    fprintf(fp, "motor_power.measured_dc_voltage_v,"
                "motor_power.calculated_dc_current_a,"
                "motor_power.motor_current_limit_arms,");

    // motor_hspeed_status_s
    fprintf(fp, "motor_speed.quadrature_current,"
                "motor_speed.direct_current,"
                "motor_speed.motor_speed,");

    // motor_safety_status_s
    fprintf(fp, "motor_safety.protection_code,"
                "motor_safety.safety_error_code,"
                "motor_safety.motor_temp,"
                "motor_safety.inverter_bridge_temp,"
                "motor_safety.bus_cap_temp,"
                "motor_safety.pwm_status,");

    // motor_control_s
    fprintf(fp, "motor_control.current_reference,"
                "motor_control.discharge_limit_pct,"
                "motor_control.charge_limit_pct,");

    // motor_error_state
    fprintf(fp, "motor_error.motor_state,");

    // pedal_s
    fprintf(fp, "pedal.pedal_supply_voltage,"
                "pedal.pedal_position_pct,"
                "pedal.pedal_raw_1,"
                "pedal.pedal_raw_2,"
                "pedal.tx_value,"
                "pedal.use_pedal,");

    // motor_protections_1_s
    fprintf(fp, "motor_prot1.can_timeout_ms,"
                "motor_prot1.dc_regen_current_limit_neg_a,"
                "motor_prot1.dc_traction_current_limit_a,"
                "motor_prot1.stall_protection_type,"
                "motor_prot1.stall_protection_time_ms,"
                "motor_prot1.stall_protection_current_a,"
                "motor_prot1.overspeed_protection_speed_rpm,");

    // motor_protections_2_s
    fprintf(fp, "motor_prot2.max_motor_temp_c,"
                "motor_prot2.motor_temp_high_gain_a_per_c,"
                "motor_prot2.max_inverter_temp_c,"
                "motor_prot2.inverter_temp_high_gain_a_per_c,"
                "motor_prot2.id_overcurrent_limit_a,"
                "motor_prot2.overvoltage_limit_v,"
                "motor_prot2.shutdown_voltage_limit_v\n");
}

/* Append one CSV row with all VSR fields (order matches csv_header above) */
static esp_err_t log_write_row(log_file_t* lf) {
    if (!lf || !lf->fp) return ESP_ERR_INVALID_ARG;

    volatile vehicle_status_reg_s* vsr = &vehicle_status_register;

    // Local snapshots of each subregister
    motor_mspeed_status_s motor_power;
    motor_hspeed_status_s motor_speed;
    motor_safety_status_s motor_safety;
    motor_control_s motor_control;
    motor_error_state motor_error;
    pedal_s pedal_data;
    motor_protections_1_s motor_prot1;
    motor_protections_2_s motor_prot2;

    // Acquire & copy (read-only semaphore)
    ACQ_REL_VSRSEM_R(motor_power, { motor_power = vsr->motor_power; });
    ACQ_REL_VSRSEM_R(motor_speed, { motor_speed = vsr->motor_speed; });
    ACQ_REL_VSRSEM_R(motor_safety, { motor_safety = vsr->motor_safety; });
    ACQ_REL_VSRSEM_R(motor_control, { motor_control = vsr->motor_control; });
    ACQ_REL_VSRSEM_R(motor_error, { motor_error = vsr->motor_error; });
    ACQ_REL_VSRSEM_R(pedal, { pedal_data = vsr->pedal; });
    ACQ_REL_VSRSEM_R(motor_prot1, { motor_prot1 = vsr->motor_prot1; });
    ACQ_REL_VSRSEM_R(motor_prot2, { motor_prot2 = vsr->motor_prot2; });

    // ---- motor_mspeed_status_s
    if (fprintf(lf->fp, "%.6f,%.6f,%.6f,", motor_power.measured_dc_voltage_v, motor_power.calculated_dc_current_a,
                motor_power.motor_current_limit_arms) < 0)
        return ESP_FAIL;

    // ---- motor_hspeed_status_s
    if (fprintf(lf->fp, "%.6f,%.6f,%" PRId16 ",", motor_speed.quadrature_current, motor_speed.direct_current,
                (int16_t) motor_speed.motor_speed) < 0)
        return ESP_FAIL;

    // ---- motor_safety_status_s
    if (fprintf(lf->fp, "%u,%u,%" PRId16 ",%" PRId16 ",%" PRId16 ",%u,", (unsigned) motor_safety.protection_code,
                (unsigned) motor_safety.safety_error_code, (int16_t) motor_safety.motor_temp,
                (int16_t) motor_safety.inverter_bridge_temp, (int16_t) motor_safety.bus_cap_temp,
                (unsigned) motor_safety.pwm_status) < 0)
        return ESP_FAIL;

    // ---- motor_control_s
    if (fprintf(lf->fp, "%" PRId32 ",%u,%u,", (int32_t) motor_control.current_reference,
                (unsigned) motor_control.discharge_limit_pct, (unsigned) motor_control.charge_limit_pct) < 0)
        return ESP_FAIL;

    // ---- motor_error_state
    if (fprintf(lf->fp, "%d,", (int) motor_error.motor_state) < 0) return ESP_FAIL;

    // ---- pedal_s
    if (fprintf(lf->fp, "%.6f,%.6f,%.6f,%.6f,%" PRId32 ",%d,", pedal_data.pedal_supply_voltage,
                pedal_data.pedal_position_pct, pedal_data.pedal_raw_1, pedal_data.pedal_raw_2,
                (int32_t) pedal_data.tx_value, (int) pedal_data.use_pedal) < 0)
        return ESP_FAIL;

    // ---- motor_protections_1_s
    if (fprintf(lf->fp, "%u,%u,%u,%u,%u,%u,%u,", (unsigned) motor_prot1.can_timeout_ms,
                (unsigned) motor_prot1.dc_regen_current_limit_neg_a, (unsigned) motor_prot1.dc_traction_current_limit_a,
                (unsigned) motor_prot1.stall_protection_type, (unsigned) motor_prot1.stall_protection_time_ms,
                (unsigned) motor_prot1.stall_protection_current_a,
                (unsigned) motor_prot1.overspeed_protection_speed_rpm) < 0)
        return ESP_FAIL;

    // ---- motor_protections_2_s (final chunk ends with \n)
    if (fprintf(lf->fp, "%u,%u,%u,%u,%u,%u,%u\n", (unsigned) motor_prot2.max_motor_temp_c,
                (unsigned) motor_prot2.motor_temp_high_gain_a_per_c, (unsigned) motor_prot2.max_inverter_temp_c,
                (unsigned) motor_prot2.inverter_temp_high_gain_a_per_c, (unsigned) motor_prot2.id_overcurrent_limit_a,
                (unsigned) motor_prot2.overvoltage_limit_v, (unsigned) motor_prot2.shutdown_voltage_limit_v) < 0)
        return ESP_FAIL;

    // Persist data & directory entry (size/time)
    log_fsync(lf->fp);
    return ESP_OK;
}

static void log_close(log_file_t* lf) {
    if (lf && lf->fp) {
        fclose(lf->fp);
        lf->fp = NULL;
    }
}

/* Example struct you can replace */
typedef struct {
    int32_t data1;
    float data2;
} sample_t;

static void logger_task(void* arg) {
    log_file_t lf;
    ESP_ERROR_CHECK(log_create_next(&lf));
    while (1) {
        if (log_write_row(&lf) != ESP_OK) {
            ESP_LOGE(TAG, "Write failed; attempting to reopen append");
            log_close(&lf);
            FILE* fp = fopen(lf.name, "a");
            if (!fp) {
                ESP_LOGE(TAG, "Reopen failed; stopping logger");
                vTaskDelete(NULL);
            }
            lf.fp = fp;
            setvbuf(lf.fp, NULL, _IOLBF, 0);
        }
        printf("wrote row\n");

        vTaskDelay(pdMS_TO_TICKS(10)); // log every 10ms
    }
}

/* =================== Static task allocation for logger ==================== */
#define LOG_STACK_WORDS (20000 / sizeof(StackType_t))
static StaticTask_t logger_task_tcb;
static StackType_t logger_task_stack[LOG_STACK_WORDS];

/* ============================== Entrypoints ================================*/

void start_logging_task(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (sd_mount("/sdcard", true) != ESP_OK) { // true => auto-format if mount fails
        printf("SD mount failed\n");
        return;
    } else {
        printf("success\n");
    }

    xTaskCreateStatic(logger_task, "logger_task", LOG_STACK_WORDS, NULL, LOGGING_TASK_PRIO, logger_task_stack,
                      &logger_task_tcb);
}
