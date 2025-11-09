// SD_reader.c — minimal CSV reader (ESP-IDF 5.3.x, ESP32 DevKitC, SDMMC 1-bit)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SD_READER";
static sdmmc_card_t *g_card = NULL;         // set by mount()

#define MAX_FILES 256
static char g_names[MAX_FILES][64];
static int  g_nums [MAX_FILES];

static esp_err_t sd_mount(const char *base_path, bool allow_format_if_needed)
{
    esp_vfs_fat_sdmmc_mount_config_t mc = {
        .format_if_mount_failed = allow_format_if_needed,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width   = 1;                    // 1-bit: CLK14, CMD15, D0=2
    slot.gpio_cd = GPIO_NUM_NC;
    slot.gpio_wp = GPIO_NUM_NC;
    slot.flags  |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    g_card = NULL;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(base_path, &host, &slot, &mc, &g_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdmmc_card_print_info(stdout, g_card);
    ESP_LOGI(TAG, "mounted at %s", base_path);
    return ESP_OK;
}

static int iequal(char a, char b){ return tolower((unsigned char)a)==tolower((unsigned char)b); }
static int starts_with_icase(const char* s, const char* pre){
    while(*pre){ if(!iequal(*s++, *pre++)) return 0; } return 1;
}
static int ends_with_icase(const char* s, const char* suf){
    size_t ls=strlen(s), lu=strlen(suf);
    if(lu>ls) return 0;
    for(size_t i=0;i<lu;i++){
        if(!iequal(s[ls-lu+i], suf[i])) return 0;
    }
    return 1;
}

/* Build list of /sdcard/dataLogX.csv (case-insensitive), print all entries for debug */
static int list_csvs(void)
{
    DIR *d = opendir("/sdcard");
    if (!d) {
        ESP_LOGE(TAG, "opendir(/sdcard) failed");
        return 0;
    }

    printf("Directory /sdcard contents:\n");
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        printf("  - %s\n", e->d_name);  // debug: show everything

        if (starts_with_icase(e->d_name, "dataLog") && ends_with_icase(e->d_name, ".csv")) {
            // Extract digits between prefix and suffix
            const char* p = e->d_name + 7; // after "dataLog"
            char numbuf[16] = {0};
            int k = 0;
            while (*p && k < (int)sizeof(numbuf)-1 && isdigit((unsigned char)*p)) {
                numbuf[k++] = *p++;
            }
            if (k > 0) {
                int X = atoi(numbuf);
                if (X >= 0 && n < MAX_FILES) {
                    g_nums[n] = X;
                    snprintf(g_names[n], sizeof(g_names[n]), "/sdcard/%.55s", e->d_name);
                    n++;
                }
            }
        }
    }
    closedir(d);

    // sort ascending by X
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            if (g_nums[j] < g_nums[i]) {
                int tn = g_nums[i]; g_nums[i] = g_nums[j]; g_nums[j] = tn;
                char tb[64]; strcpy(tb, g_names[i]); strcpy(g_names[i], g_names[j]); strcpy(g_names[j], tb);
            }
    return n;
}

static esp_err_t stream_csv_number(int X)
{
    char path[64];
    snprintf(path, sizeof(path), "/sdcard/dataLog%d.csv", X);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        ESP_LOGE(TAG, "open failed: %s", path);
        return ESP_FAIL;
    }

    printf("\n--- BEGIN %s ---\n", path);
    char buf[512];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), fp)) > 0)
        fwrite(buf, 1, r, stdout);
    fclose(fp);
    printf("\n--- END %s ---\n", path);
    return ESP_OK;
}

#include <errno.h>   // <-- add if not already included

// Remove "/sdcard/<name>" or a full path the user typed.
// Prints result; safe against long names.
static void rm_csv(const char *user_name)
{
    if (!user_name) { printf("usage: rm <fileName.csv>\n"); return; }

    // Skip leading spaces
    while (*user_name == ' ') user_name++;

    // Build path: accept either bare name or full path
    char path[96];
    if (strchr(user_name, '/')) {
        snprintf(path, sizeof(path), "%.95s", user_name);            // full path given
    } else {
        snprintf(path, sizeof(path), "/sdcard/%.86s", user_name);    // prepend /sdcard/
    }

    // Trim trailing CR/LF/spaces from path (user pressed Enter)
    size_t L = strcspn(path, "\r\n");
    while (L && (path[L-1] == ' ' || path[L-1] == '\t')) L--;
    path[L] = '\0';

    if (remove(path) == 0) {
        printf("Removed %s\n", path);
    } else {
        printf("Remove failed for %s (errno=%d)\n", path, errno);
    }
}

#include "driver/uart.h"
#include "esp_vfs_dev.h"
static void enable_console_input(void) {
    const int uart_num = UART_NUM_0;
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // RX buffer so fgets() can block and read
    ESP_ERROR_CHECK(uart_driver_install(uart_num, /*rx_buffer_size*/ 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &cfg));
    // Attach stdin/stdout/stderr to this UART driver
    esp_vfs_dev_uart_use_driver(uart_num);
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
        enable_console_input();          

    if (sd_mount("/sdcard", false) != ESP_OK) {
        printf("SD mount failed. If the card is exFAT/blank, rebuild with allow_format_if_needed=true and run once.\n");
        return;
    }

    int n = list_csvs();
    if (n == 0) {
        printf("No dataLogX.csv files found.\n");
        return;
    }

    printf("\n=== SD CSV Reader ===\nFound %d file(s):\n", n);
    for (int i = 0; i < n; ++i) {
        struct stat st;
        if (stat(g_names[i], &st) == 0)
            printf("  [%d] %s  (size=%ld bytes)\n", g_nums[i], g_names[i], (long)st.st_size);
        else
            printf("  [%d] %s  (size=?; stat failed)\n", g_nums[i], g_names[i]);
    }
    printf("\nEnter X (from dataLogX.csv) or 'rm fileName.csv' and press Enter: ");

    printf("\nEnter X (from dataLogX.csv) and press Enter: ");
    char line[32];
    while(1){
        if (!fgets(line, sizeof(line), stdin)) return;
        size_t len = strcspn(line, "\r\n");
        line[len] = '\0';
        if (len == 0) continue;              // ignore blank lines
        // If command starts with "rm " (case-insensitive), delete and refresh list
        if ((line[0] == 'r' || line[0] == 'R') &&
            (line[1] == 'm' || line[1] == 'M') &&
            (line[2] == ' ')) {

            rm_csv(line + 3);

            // Rebuild list and reprint with sizes
            int n = list_csvs();
            if (n == 0) {
                printf("No dataLogX.csv files found.\n");
            } else {
                printf("\nUpdated file list (%d):\n", n);
                for (int i = 0; i < n; ++i) {
                    struct stat st;
                    if (stat(g_names[i], &st) == 0)
                        printf("  [%d] %s  (size=%ld bytes)\n", g_nums[i], g_names[i], (long)st.st_size);
                    else
                        printf("  [%d] %s  (size=?; stat failed)\n", g_nums[i], g_names[i]);
                }
            }
            printf("\nEnter X (from dataLogX.csv) or 'rm fileName.csv' and press Enter: ");
            continue;
        }
        int X = atoi(line);

        if (stream_csv_number(X) != ESP_OK)
            printf("Failed to stream dataLog%d.csv\n", X);
    }

}
