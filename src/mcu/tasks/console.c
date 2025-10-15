// Minimal ESP-IDF console REPL (UART0) with
#include "driver/uart.h"
#include "esp_clk_tree.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/clk_tree_defs.h"
#include "tasks.h"
#include <esp_chip_info.h>
#include <stdio.h>

#define CONSOLE_MAX_TASKS 64U

typedef union {
    char text[4096];
    TaskStatus_t task_status[CONSOLE_MAX_TASKS];
} console_scratch_t;

// Reused scratch space to avoid per-command heap usage.
static console_scratch_t console_scratch;

char* mila_text = "\n      __  __ ___ _        _    "
                  "\n     |  \\/  |_ _| |      / \\   "
                  "\n     | |\\/| || || |     / _ \\  "
                  "\n     | |  | || || |___ / ___ \\ "
                  "\n     |_|  |_|___|_____/_/   \\_\\ "
                  "\n ";

// Redirect logging
static int uart_log_vprintf(const char* fmt, va_list ap) {
    char buf[256];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    static const char* kPrompt = "mcu> ";
    if (n > 0) {
        uart_write_bytes(UART_NUM_0, buf, n);                                           // same UART the console uses
        if (buf[n - 1] == '\n') uart_write_bytes(UART_NUM_0, kPrompt, strlen(kPrompt)); // optional prompt redraw
    }
    return n;
}

// ---- basic commands ----
static int cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) printf("%s%s", argv[i], (i + 1 < argc) ? " " : "\n");
    return 0;
}
static int cmd_free(int argc, char** argv) {
    (void) argc;
    (void) argv;
    printf("free heap: %u bytes\n", (unsigned) esp_get_free_heap_size());
    return 0;
}

// ---- status commands ----

// Map FreeRTOS task state enum to a compact display code.
static char task_state_char(eTaskState state) {
    switch (state) {
        case eRunning: return 'R';
        case eReady: return 'r';
        case eBlocked: return 'B';
        case eSuspended: return 'S';
        case eDeleted: return 'D';
        default: return '?';
    }
}

// "top" -> system info + per-task CPU/stack stats
static int cmd_top(int argc, char** argv) {
    (void) argc;
    (void) argv;
    ESP_LOGE(__func__, "LOG ERROR TEST");
    uint64_t us = esp_timer_get_time();

    uint32_t cpu_hz = 0, apb_hz = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &cpu_hz);
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &apb_hz);

    esp_chip_info_t info;
    esp_chip_info(&info);
    printf("uptime: %.3fs\n", (double) us / 1e6);
    printf("cpu: %lu Hz  apb: %lu Hz\n", cpu_hz, apb_hz);
    printf("chip: cores=%d  rev=%d\n", info.cores, info.revision);

    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    if (task_count > CONSOLE_MAX_TASKS) {
        printf("too many tasks (%lu) for stats buffer (%u max)\n", (unsigned long) task_count,
               (unsigned) CONSOLE_MAX_TASKS);
        return 1;
    }

    uint32_t total_runtime = 0;
    UBaseType_t fetched = uxTaskGetSystemState(console_scratch.task_status, CONSOLE_MAX_TASKS, &total_runtime);
    if (fetched == 0) {
        puts("failed to collect task stats");
        return 1;
    }

    if (total_runtime == 0) {
        for (UBaseType_t i = 0; i < fetched; ++i) total_runtime += console_scratch.task_status[i].ulRunTimeCounter;
        if (total_runtime == 0) total_runtime = 1;
    }

    puts("Task             St Prio Stack(B)  CPU%   AbsTime");
    for (UBaseType_t i = 0; i < fetched; ++i) {
        const TaskStatus_t* st = &console_scratch.task_status[i];
        double cpu_pct = ((double) st->ulRunTimeCounter * 100.0) / (double) total_runtime;
        unsigned stack_bytes = (unsigned) st->usStackHighWaterMark * sizeof(StackType_t);
        printf("%-16s %c %4u %8u %6.2f %10lu\n", st->pcTaskName, task_state_char(st->eCurrentState),
               (unsigned) st->uxCurrentPriority, stack_bytes, cpu_pct, (unsigned long) st->ulRunTimeCounter);
    }
    return 0;
}

static void register_cmds(void) {
    const esp_console_cmd_t cmds[] = {
        {.command = "echo", .help = "Echo args back", .func = &cmd_echo},
        {.command = "free", .help = "Show free heap", .func = &cmd_free},
        {.command = "top", .help = "System info + per-task CPU/stack", .func = &cmd_top},
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i) ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
}

static void console_main(void* arg) {
    (void) arg;

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "mcu> ";
    repl_cfg.max_cmdline_length = 256;

    esp_console_dev_uart_config_t dev_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    dev_cfg.channel = UART_NUM_0;
    dev_cfg.baud_rate = 115200;

    esp_console_repl_t* repl = NULL;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&dev_cfg, &repl_cfg, &repl));

    register_cmds();

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    esp_log_set_vprintf(&uart_log_vprintf);
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("%s", mila_text);
    vTaskDelete(NULL);
}

// ---- required external entrypoint ----
void start_console_task() {
    static StackType_t stack[DEFAULT_STACK_SIZE];
    static StaticTask_t tcb;
    xTaskCreateStaticPinnedToCore(console_main, "console_main", DEFAULT_STACK_SIZE, NULL, CONSOLE_TASK_PRIO, stack,
                                  &tcb, 0);
}
