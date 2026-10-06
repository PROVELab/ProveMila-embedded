#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void app_main(void) {
    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NO_ACK);
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    printf("CANTEST install: %s\n", esp_err_to_name(twai_driver_install(&g, &t, &f)));
    printf("CANTEST start: %s\n", esp_err_to_name(twai_start()));
    twai_reconfigure_alerts(TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_RX_DATA | TWAI_ALERT_BUS_OFF, NULL);

    for (int i = 0; i < 3; i++) {
        twai_message_t m = {.identifier = 0x123, .data_length_code = 2, .data = {0xAB, i}, .self = 1};
        printf("CANTEST transmit #%d: %s\n", i, esp_err_to_name(twai_transmit(&m, pdMS_TO_TICKS(500))));

        uint32_t alerts = 0;
        esp_err_t ar = twai_read_alerts(&alerts, pdMS_TO_TICKS(1000));
        printf("CANTEST alerts: %s 0x%lx\n", esp_err_to_name(ar), (unsigned long)alerts);

        twai_message_t rx;
        esp_err_t rr = twai_receive(&rx, pdMS_TO_TICKS(1000));
        if (rr == ESP_OK) {
            printf("CANTEST received id=0x%lx data=%02x %02x\n", (unsigned long)rx.identifier, rx.data[0], rx.data[1]);
        } else {
            printf("CANTEST receive: %s\n", esp_err_to_name(rr));
        }
    }

    twai_status_info_t st;
    twai_get_status_info(&st);
    printf("CANTEST status: state=%d tx_err=%lu msgs_to_tx=%lu msgs_to_rx=%lu\n", st.state, (unsigned long)st.tx_error_counter,
           (unsigned long)st.msgs_to_tx, (unsigned long)st.msgs_to_rx);
    printf("CANTEST done\n");
}
