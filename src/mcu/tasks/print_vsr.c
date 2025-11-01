#include "print_vsr.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "freertos/semphr.h"

static void print_timestamp_info(const char* indent, int64_t timestamp_us) {
    int64_t age_ms = 0;
    if (timestamp_us > 0) {
        int64_t now_us = esp_timer_get_time();
        if (now_us > timestamp_us) age_ms = (now_us - timestamp_us) / 1000;
    }
    const char* note = timestamp_us > 0 ? "" : ", not updated yet";
    printf("%stimestamp: %" PRIi64 " us (%" PRIi64 " ms ago%s)\n", indent, timestamp_us, age_ms, note);
}

static void print_motor_power(volatile vehicle_status_reg_s* vsr) {
    motor_mspeed_status_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_power, {
        data = vsr->motor_power;
        timestamp_us = vsr->motor_power_timestamp;
    });
    printf("motor_power:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  measured_dc_voltage_v: %.3f\n", (double) data.measured_dc_voltage_v);
    printf("  calculated_dc_current_a: %.3f\n", (double) data.calculated_dc_current_a);
    printf("  motor_current_limit_arms: %.3f\n", (double) data.motor_current_limit_arms);
}

static void print_motor_speed(volatile vehicle_status_reg_s* vsr) {
    motor_hspeed_status_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_speed, {
        data = vsr->motor_speed;
        timestamp_us = vsr->motor_speed_timestamp;
    });
    printf("motor_speed:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  quadrature_current: %.3f\n", (double) data.quadrature_current);
    printf("  direct_current: %.3f\n", (double) data.direct_current);
    printf("  motor_speed: %" PRId16 "\n", data.motor_speed);
}

static void print_motor_safety(volatile vehicle_status_reg_s* vsr) {
    motor_safety_status_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_safety, {
        data = vsr->motor_safety;
        timestamp_us = vsr->motor_safety_timestamp;
    });
    printf("motor_safety:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  protection_code: %" PRIu8 "\n", data.protection_code);
    printf("  safety_error_code: %" PRIu8 "\n", data.safety_error_code);
    printf("  motor_temp: %" PRId16 "\n", data.motor_temp);
    printf("  inverter_bridge_temp: %" PRId16 "\n", data.inverter_bridge_temp);
    printf("  bus_cap_temp: %" PRId16 "\n", data.bus_cap_temp);
    printf("  pwm_status: %" PRIu8 "\n", data.pwm_status);
}

static void print_motor_control(volatile vehicle_status_reg_s* vsr) {
    motor_control_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_control, {
        data = vsr->motor_control;
        timestamp_us = vsr->motor_control_timestamp;
    });
    printf("motor_control:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  speed_reference (in 8ths): %" PRId32 "\n", data.current_reference);
    printf("  discharge_limit_pct: %" PRIu8 "\n", data.discharge_limit_pct);
    printf("  charge_limit_pct: %" PRIu8 "\n", data.charge_limit_pct);
}

static const char* motor_state_to_str(high_level_motor_state state) {
    switch (state) {
        case MOTOR_OK: return "MOTOR_OK";
        case MOTOR_ERROR_STOP: return "MOTOR_ERROR_STOP";
        default: return "UNKNOWN";
    }
}

static void print_motor_error(volatile vehicle_status_reg_s* vsr) {
    motor_error_state data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_error, {
        data = vsr->motor_error;
        timestamp_us = vsr->motor_error_timestamp;
    });
    printf("motor_error:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  motor_state: %s (%d)\n", motor_state_to_str(data.motor_state), (int) data.motor_state);
}

static void print_motor_prot1(volatile vehicle_status_reg_s* vsr) {
    motor_protections_1_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_prot1, {
        data = vsr->motor_prot1;
        timestamp_us = vsr->motor_prot1_timestamp;
    });
    printf("motor_prot1:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  can_timeout_ms: %" PRIu16 "\n", data.can_timeout_ms);
    printf("  dc_regen_current_limit_a (neg): %" PRIu16 "\n", data.dc_regen_current_limit_neg_a);
    printf("  dc_traction_current_limit_a: %" PRIu16 "\n", data.dc_traction_current_limit_a);
    printf("  stall_protection_type: %" PRIu8 "\n", data.stall_protection_type);
    printf("  stall_protection_time_ms: %" PRIu16 "\n", data.stall_protection_time_ms);
    printf("  stall_protection_current_a: %" PRIu16 "\n", data.stall_protection_current_a);
    printf("  overspeed_protection_speed_rpm: %" PRIu8 "\n", data.overspeed_protection_speed_rpm);
}

static void print_motor_prot2(volatile vehicle_status_reg_s* vsr) {
    motor_protections_2_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(motor_prot2, {
        data = vsr->motor_prot2;
        timestamp_us = vsr->motor_prot2_timestamp;
    });
    printf("motor_prot2:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  max_motor_temp_c: %" PRIu8 "\n", data.max_motor_temp_c);
    printf("  motor_temp_high_gain_a_per_c: %" PRIu8 "\n", data.motor_temp_high_gain_a_per_c);
    printf("  max_inverter_temp_c: %" PRIu8 "\n", data.max_inverter_temp_c);
    printf("  inverter_temp_high_gain_a_per_c: %" PRIu8 "\n", data.inverter_temp_high_gain_a_per_c);
    printf("  id_overcurrent_limit_2a: %" PRIu16 "\n", data.id_overcurrent_limit_a);
    printf("  overvoltage_limit_2v: %" PRIu16 "\n", data.overvoltage_limit_v);
    printf("  shutdown_voltage_limit_2v: %" PRIu16 "\n", data.shutdown_voltage_limit_v);
}

static void print_pedal(volatile vehicle_status_reg_s* vsr) {
    pedal_s data;
    int64_t timestamp_us = 0;
    ACQ_REL_VSRSEM_R(pedal, {
        data = vsr->pedal;
        timestamp_us = vsr->pedal_timestamp;
    });
    printf("pedal:\n");
    print_timestamp_info("  ", timestamp_us);
    printf("  pedal_supply_voltage: %.3fmV\n", (double) data.pedal_supply_voltage);
    printf("  pedal_position_pct: %.3f\n", (double) data.pedal_position_pct);
    printf("  [pedal_raw_1, pedal_raw_2]: [%.3f, %.3f]\n", (double) data.pedal_raw_1, (double) data.pedal_raw_2);
    printf("  tx_value: %" PRId32 "\n", data.tx_value);
    printf("  use_pedal: %s\n", data.use_pedal ? "true" : "false");
}

static const vsr_topic_printer_t kTopics[] = {
    {.name = "motor_power", .print = &print_motor_power},   {.name = "motor_speed", .print = &print_motor_speed},
    {.name = "motor_safety", .print = &print_motor_safety}, {.name = "motor_control", .print = &print_motor_control},
    {.name = "motor_error", .print = &print_motor_error},   {.name = "motor_prot1", .print = &print_motor_prot1},
    {.name = "motor_prot2", .print = &print_motor_prot2},   {.name = "pedal", .print = &print_pedal},
};

const vsr_topic_printer_t* vsr_find_topic_printer(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < sizeof(kTopics) / sizeof(kTopics[0]); ++i) {
        if (strcmp(kTopics[i].name, name) == 0) return &kTopics[i];
    }
    return NULL;
}

void vsr_print_topic(const vsr_topic_printer_t* topic, volatile vehicle_status_reg_s* vsr) {
    if (!topic || !vsr) return;
    topic->print(vsr);
}

void vsr_print_all_topics(volatile vehicle_status_reg_s* vsr) {
    if (!vsr) return;
    for (size_t i = 0; i < sizeof(kTopics) / sizeof(kTopics[0]); ++i) {
        kTopics[i].print(vsr);
        if (i + 1 < sizeof(kTopics) / sizeof(kTopics[0])) puts("");
    }
}

void vsr_print_available_topics(void) {
    printf("available topics:\n");
    for (size_t i = 0; i < sizeof(kTopics) / sizeof(kTopics[0]); ++i) { printf("  %s\n", kTopics[i].name); }
}
