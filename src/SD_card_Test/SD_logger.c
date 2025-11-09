#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "tasks.h"  // defines LOGGING_TASK_PRIO (fallback provided below if missing)

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#ifndef LOGGING_TASK_PRIO
#define LOGGING_TASK_PRIO 5
#endif

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "diskio_impl.h"   // for ff_diskio_register_sdmmc (ESP-IDF 5.3.x)

#include <unistd.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>

static const char *TAG = "SDFMT";
static sdmmc_card_t* g_card;  // must live for the life of the mount

esp_err_t sd_mount(const char *base_path, bool allow_format_if_needed)
{
    esp_vfs_fat_sdmmc_mount_config_t mc = {
        .format_if_mount_failed = allow_format_if_needed,  // true => auto-FAT32 if needed
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;  // 400 kHz for bring-up

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width   = 1;                        // 1-bit: CLK14, CMD15, D0=2
    slot.gpio_cd = GPIO_NUM_NC;
    slot.gpio_wp = GPIO_NUM_NC;
    slot.flags  |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    g_card = NULL;  // mount will set this
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(base_path, &host, &slot, &mc, &g_card);
    if (ret != ESP_OK) {
        ESP_LOGE("SD", "mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdmmc_card_print_info(stdout, g_card);   // expects sdmmc_card_t*
    ESP_LOGI("SD", "mounted at %s", base_path);
    return ESP_OK;
}

esp_err_t sd_unmount(const char *base_path)
{
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(base_path, g_card);
    g_card = NULL;
    return ret;
}


static inline void log_fsync(FILE *fp) {
    fflush(fp);
    fsync(fileno(fp));   // forces FAT directory entry (size/time) to be updated
}

typedef struct {
    FILE *fp;
    char  name[48];
} log_file_t;

static bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

/* Create first missing /sdcard/dataLogX.csv and write header */
static esp_err_t log_create_next(log_file_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    out->fp = NULL;
    out->name[0] = '\0';

    for (int i = 0; i < 100000; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/sdcard/dataLog%d.csv", i);
        if (!file_exists(path)) {
            FILE *fp = fopen(path, "w");
            if (!fp) {
                ESP_LOGE(TAG, "Failed to create %s", path);
                return ESP_FAIL;
            }
            fprintf(fp, "data1,data2\n");   // header for example struct
            log_fsync(fp);
            setvbuf(fp, NULL, _IOLBF, 0);   // line-buffered
            out->fp = fp;
            strncpy(out->name, path, sizeof(out->name)-1);
            out->name[sizeof(out->name)-1] = '\0';
            ESP_LOGI(TAG, "Created %s", out->name);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

/* Append a CSV row "data1,data2\n" */
static esp_err_t log_write_row(log_file_t *lf, int32_t data1, float data2)
{
    if (!lf || !lf->fp) return ESP_ERR_INVALID_ARG;
    if (fprintf(lf->fp, "%" PRId32 ",%.6f\n", data1, data2) < 0) return ESP_FAIL;
    log_fsync(lf->fp);
    fflush(lf->fp);
    return ESP_OK;
}

static void log_close(log_file_t *lf)
{
    if (lf && lf->fp) {
        fclose(lf->fp);
        lf->fp = NULL;
    }
}

/* Example struct you can replace */
typedef struct {
    int32_t data1;
    float   data2;
} sample_t;

static void logger_task(void *arg)
{
    log_file_t lf;
    ESP_ERROR_CHECK(log_create_next(&lf));

    sample_t s = {.data1 = 0, .data2 = 0.0f};

    while (1) {
        // If you share this struct with other tasks, guard with your mutex.
        s.data1++;
        s.data2 += 0.5f;

        if (log_write_row(&lf, s.data1, s.data2) != ESP_OK) {
            ESP_LOGE(TAG, "Write failed; attempting to reopen append");
            log_close(&lf);
            FILE *fp = fopen(lf.name, "a");
            if (!fp) {
                ESP_LOGE(TAG, "Reopen failed; stopping logger");
                vTaskDelete(NULL);
            }
            lf.fp = fp;
            setvbuf(lf.fp, NULL, _IOLBF, 0);
        }
        printf("wrote row\n");

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz
    }
}

/* =================== Static task allocation for logger ==================== */
#define LOG_STACK_WORDS (20000 / sizeof(StackType_t))
static StaticTask_t logger_task_tcb;
static StackType_t logger_task_stack[LOG_STACK_WORDS];

/* ============================== Entrypoints ================================*/

void start_logging_task(void){
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (sd_mount("/sdcard", true) != ESP_OK) {   // true => auto-format if mount fails
        printf("SD mount failed\n");
        return;
    }else{
        printf("success\n");
    }

    xTaskCreateStatic(
        logger_task,
        "logger_task",
        LOG_STACK_WORDS,
        NULL,
        LOGGING_TASK_PRIO,
        logger_task_stack,
        &logger_task_tcb
    );
}