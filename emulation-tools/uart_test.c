#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#ifndef NODE_ID
#define NODE_ID 0
#endif

void app_main(void) {
    uart_config_t cfg = {.baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
                         .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT};
    printf("UARTTEST node %d install: %s\n", NODE_ID, esp_err_to_name(uart_driver_install(UART_NUM_1, 512, 512, 0, NULL, 0)));
    uart_param_config(UART_NUM_1, &cfg);
    uart_set_pin(UART_NUM_1, 25, 26, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    for (int i = 0; i < 10; i++) {
        char out[32];
        int n = snprintf(out, sizeof out, "PING node%d #%d\n", NODE_ID, i);
        int w = uart_write_bytes(UART_NUM_1, out, n);
        printf("UARTTEST node %d tx %d bytes\n", NODE_ID, w);

        char in[64];
        int r = uart_read_bytes(UART_NUM_1, in, sizeof in - 1, pdMS_TO_TICKS(1000));
        if (r > 0) {
            in[r] = 0;
            for (char* p = in; *p; p++) if (*p == '\n') *p = '|';
            printf("UARTTEST node %d rx: %s\n", NODE_ID, in);
        }
    }
    printf("UARTTEST node %d done\n", NODE_ID);
}
