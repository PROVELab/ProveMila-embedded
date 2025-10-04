
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_system.h"
#include "esp_vfs_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Seems like too much work to setup the actual espidf console tbh

// ---- UART0 (USB) console setup ----
static void console_init(void) {
    const uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // Install UART0 driver (RX buffer only; TX can be blocking)
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    // Normalize line endings so CRLF from terminals becomes '\n'
    esp_vfs_dev_uart_port_set_rx_line_endings(UART_NUM_0,
                                              ESP_LINE_ENDINGS_CRLF);
    esp_vfs_dev_uart_port_set_tx_line_endings(UART_NUM_0,
                                              ESP_LINE_ENDINGS_CRLF);

    // Route stdin/stdout/stderr to the UART driver
    esp_vfs_dev_uart_use_driver(UART_NUM_0);

    // Make stdio unbuffered so prints appear immediately
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

// ---- tiny command framework ----
typedef int (*cmd_fn)(int argc, char **argv);
typedef struct {
    const char *name;
    const char *help;
    cmd_fn fn;
} cmd_t;

// forward decls
static int cmd_help(int argc, char **argv);
static int cmd_echo(int argc, char **argv);
static int cmd_add(int argc, char **argv);
static int cmd_free(int argc, char **argv);

static const cmd_t CMDS[] = {
    {"help", "List commands", cmd_help},
    {"echo", "Echo args back", cmd_echo},
    {"add", "add A B  -> prints A+B", cmd_add},
    {"free", "Show free heap", cmd_free},
};

static int cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); ++i) {
        printf("%-8s - %s\n", CMDS[i].name, CMDS[i].help);
    }
    return 0;
}
static int cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        printf("%s%s", argv[i], (i + 1 < argc) ? " " : "\n");
    }
    return 0;
}
static int cmd_add(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: add A B\n");
        return 1;
    }
    long a = strtol(argv[1], NULL, 0);
    long b = strtol(argv[2], NULL, 0);
    printf("%ld\n", a + b);
    return 0;
}
static int cmd_free(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("free heap: %u bytes\n", (unsigned)esp_get_free_heap_size());
    return 0;
}

// split a line into argv (very simple: whitespace-delimited)
static int split_args(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args) {
        while (isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && !isspace((unsigned char)*p))
            p++;
        if (*p)
            *p++ = '\0';
    }
    return argc;
}

void console_main(void) {
    console_init();
    printf("\nMCU repl ready. type 'help'.\n");

    char line[256];
    char *argv[16];

    while (true) {
        printf("mcu> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            // if something odd happens, just continue
            continue;
        }

        // trim trailing \r\n
        size_t n = strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = 0;
        if (n == 0)
            continue;

        int argc = split_args(line, argv, 16);
        if (argc == 0)
            continue;

        // dispatch
        bool handled = false;
        for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); ++i) {
            if (strcmp(argv[0], CMDS[i].name) == 0) {
                CMDS[i].fn(argc, argv);
                handled = true;
                break;
            }
        }
        if (!handled) {
            printf("unknown command: %s (try 'help')\n", argv[0]);
        }
    }
}

void 
