// espPower.c
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_err.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"

#include "../../espBase/debug_esp.h" // mutexPrint()
#include "powerSensor.h"

// ============================================================================
//                      Static sizing (fixed totals = 512)
// ============================================================================

#define MIN_FREQUENCY_HZ 20000u // 20 kHz total stream

// Fixed total samples in the drain window (shared across all channels)
#define ADC_TOTAL_SAMPLES      512u
#define ADC_CONV_FRAME_SAMPLES (ADC_TOTAL_SAMPLES / 4u) // 128 samples/frame
#define ADC_READ_BUF_SAMPLES   (ADC_TOTAL_SAMPLES)

#define ADC_SAMPLE_BYTES     (sizeof(adc_digi_output_data_t))
#define ADC_POOL_BYTES       (ADC_TOTAL_SAMPLES * ADC_SAMPLE_BYTES)
#define ADC_CONV_FRAME_BYTES (ADC_CONV_FRAME_SAMPLES * ADC_SAMPLE_BYTES)
#define ADC_READ_BUF_BYTES   (ADC_READ_BUF_SAMPLES * ADC_SAMPLE_BYTES)

_Static_assert(ADC_READ_BUF_SAMPLES >= ADC_CONV_FRAME_SAMPLES, "read buf must be >= frame");
_Static_assert((ADC_POOL_BYTES % 4) == 0, "pool must be 4-byte aligned");

// Use DB_12 attenuation and 12-bit width
static const adc_atten_t defaultAtten = ADC_ATTEN_DB_12;
static const adc_bitwidth_t defaultBitWidth = ADC_BITWIDTH_12;

// ============================================================================
//                           Module state / helpers
// ============================================================================

// Internal cap for table sizing (static, not exposed). Big enough for ESP32 ADC1/2.
#define MAX_CHANNELS 16

typedef struct {
    bool enabled;
    int gpio;
    adc_channel_t ch;
    float divider_gain;       // (R1+R2)/R2
    adc_cali_handle_t cali_h; // NULL if not calibrated
} slot_t;

static struct {
    bool initialized;
    adc_continuous_handle_t adc_h;
    adc_unit_t unit;           // ADC_UNIT_1 or ADC_UNIT_2
    int total_slots;           // how many entries the app asked for
    int enabled_slots;         // how many were valid/enabled
    slot_t slot[MAX_CHANNELS]; // entries stay in the same order as provided
} S = {0};

DMA_ATTR static uint8_t s_adc_read_buf[ADC_READ_BUF_BYTES];

// ADC1: channels 0..7. We should prefer 36 and 39 (VP and VN),
// since those are solely for ADC use, and a flux sensor we don't use.
static bool gpio_to_adc1_channel(int gpio, adc_channel_t* ch_out) {
    switch (gpio) {
        case VP_Pin: *ch_out = ADC_CHANNEL_0; return true; // pin 36. Use Me if Unsure!
        case 37: *ch_out = ADC_CHANNEL_1; return true;     // other channels like me are not exposed on board
        case 38: *ch_out = ADC_CHANNEL_2; return true;
        case VN_Pin: *ch_out = ADC_CHANNEL_3; return true; // pin 39. Use Me if Unsure!
        case 32: *ch_out = ADC_CHANNEL_4; return true;
        case 33: *ch_out = ADC_CHANNEL_5; return true;
        case 34: *ch_out = ADC_CHANNEL_6; return true; // use us if alr using 36/39. were directly on board
        case 35: *ch_out = ADC_CHANNEL_7; return true; // use us if alr using 36/39. were directly on board
        default: return false;
    }
}

// ADC2 (conflicts with Wi-Fi on ESP32): channels 0..9
static bool gpio_to_adc2_channel(int gpio, adc_channel_t* ch_out) {
    switch (gpio) {
        case 4: *ch_out = ADC_CHANNEL_0; return true;  // ADC2_CH0
        case 0: *ch_out = ADC_CHANNEL_1; return true;  // ADC2_CH1
        case 2: *ch_out = ADC_CHANNEL_2; return true;  // ADC2_CH2
        case 15: *ch_out = ADC_CHANNEL_3; return true; // ADC2_CH3
        case 13: *ch_out = ADC_CHANNEL_4; return true; // ADC2_CH4
        case 12: *ch_out = ADC_CHANNEL_5; return true; // ADC2_CH5
        case 14: *ch_out = ADC_CHANNEL_6; return true; // ADC2_CH6
        case 27: *ch_out = ADC_CHANNEL_7; return true; // ADC2_CH7
        case 25: *ch_out = ADC_CHANNEL_8; return true; // ADC2_CH8
        case 26: *ch_out = ADC_CHANNEL_9; return true; // ADC2_CH9
        default: return false;
    }
}

static inline int channel_to_slot(adc_channel_t ch) {
    // Search within total slots; only enabled ones will match
    for (int i = 0; i < S.total_slots; ++i)
        if (S.slot[i].enabled && S.slot[i].ch == ch) return i;
    return -1;
}

static bool init_calibration_for_slot(int idx) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg_cf = {
        .unit_id = S.unit,
        .chan = S.slot[idx].ch,
        .atten = defaultAtten,
        .bitwidth = defaultBitWidth,
    };
    if (adc_cali_create_scheme_curve_fitting(&cfg_cf, &S.slot[idx].cali_h) == ESP_OK) {
        char b[96];
        snprintf(b, sizeof(b), "ADC calibration: curve-fitting enabled (slot=%d)\n", idx);
        mutexPrint(b);
        return true;
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg_lf = {
        .unit_id = S.unit,
        .atten = defaultAtten,
        .bitwidth = defaultBitWidth,
#if CONFIG_IDF_TARGET_ESP32
        .default_vref = 1100,
#endif
    };
    if (adc_cali_create_scheme_line_fitting(&cfg_lf, &S.slot[idx].cali_h) == ESP_OK) {
        char b[96];
        snprintf(b, sizeof(b), "ADC calibration: line-fitting enabled (slot=%d)\n", idx);
        mutexPrint(b);
        return true;
    }
#endif
    S.slot[idx].cali_h = NULL;
    return false;
}

int getSelfPowerChannelCount(void) { return S.total_slots; }

// ============================================================================
//                                   INIT
// ============================================================================

void initializeSelfPower(const selfPowerConfig* configs, int num_channels, int adc_unit,
                         selfPowerStatus_t* out_statuses) {
    // Defensive defaults for caller
    if (out_statuses) {
        for (int i = 0; i < num_channels; ++i) out_statuses[i] = INIT_FAILURE;
    }

    if (!configs || num_channels <= 0 || num_channels > MAX_CHANNELS) {
        mutexPrint("initializeSelfPower: bad args or too many channels\n");
        return;
    }
    if (S.initialized) {
        mutexPrint("SelfPower already initialized\n");
        return;
    }

    // Select and store ADC unit
    S.unit = (adc_unit == 2) ? ADC_UNIT_2 : ADC_UNIT_1;

    S.total_slots = num_channels;
    S.enabled_slots = 0;

    adc_digi_pattern_config_t patterns[MAX_CHANNELS] = {0};

    // Validate each requested channel; keep order; mark per-channel status
    for (int i = 0; i < num_channels; ++i) {
        const selfPowerConfig* c = &configs[i];
        slot_t* sl = &S.slot[i];

        sl->enabled = false;
        sl->gpio = c->ADCPin;
        sl->divider_gain = 0.0f;
        sl->cali_h = NULL;

        if (c->R1 <= 0 || c->R2 <= 0) {
            mutexPrint("Invalid R1/R2 divider values\n");
            continue;
        }

        adc_channel_t ch;
        bool ok = false;
        if (S.unit == ADC_UNIT_1) {
            ok = gpio_to_adc1_channel(c->ADCPin, &ch);
        } else {
            ok = gpio_to_adc2_channel(c->ADCPin, &ch);
        }
        if (!ok) {
            mutexPrint("ADC GPIO invalid for selected ADC unit\n");
            continue;
        }

        sl->enabled = true;
        sl->ch = ch;
        sl->divider_gain = ((float) c->R1 + (float) c->R2) / (float) c->R2;

        patterns[S.enabled_slots].atten = defaultAtten;
        patterns[S.enabled_slots].channel = ch;
        patterns[S.enabled_slots].unit = S.unit;
        patterns[S.enabled_slots].bit_width = defaultBitWidth;

        if (out_statuses) out_statuses[i] = SUCCESSFUL_INIT_NO_CALIBRATION; // provisional
        S.enabled_slots++;
    }

    if (S.enabled_slots == 0) {
        mutexPrint("No valid ADC channels to configure\n");
        return;
    }

    // 1) Handle
    adc_continuous_handle_cfg_t hcfg = {
        .max_store_buf_size = ADC_POOL_BYTES,
        .conv_frame_size = ADC_CONV_FRAME_BYTES,
    };
    if (adc_continuous_new_handle(&hcfg, &S.adc_h) != ESP_OK) {
        mutexPrint("adc_continuous_new_handle failed\n");
        return;
    }

    // 2) Config with selected unit + multi-pattern
    adc_continuous_config_t cfg = {
        .sample_freq_hz = MIN_FREQUENCY_HZ,
        .conv_mode = (S.unit == ADC_UNIT_1) ? ADC_CONV_SINGLE_UNIT_1 : ADC_CONV_SINGLE_UNIT_2,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = S.enabled_slots,
        .adc_pattern = patterns,
    };
    if (adc_continuous_config(S.adc_h, &cfg) != ESP_OK) {
        mutexPrint("adc_continuous_config failed\n");
        return;
    }
    if (adc_continuous_start(S.adc_h) != ESP_OK) {
        mutexPrint("adc_continuous_start failed\n");
        return;
    }

    // 3) Per-channel calibration + set final per-channel init status
    for (int i = 0; i < num_channels; ++i) {
        if (!S.slot[i].enabled) {
            if (out_statuses) out_statuses[i] = INIT_FAILURE;
            continue;
        }
        bool cal = init_calibration_for_slot(i);
        if (out_statuses) { out_statuses[i] = cal ? SUCCESSFUL_INIT_CALIBRATED : SUCCESSFUL_INIT_NO_CALIBRATION; }
    }

    S.initialized = true;

    // Log summary in config order
    for (int i = 0; i < num_channels; ++i) {
        if (!S.slot[i].enabled) continue;
        char buffer[192];
        // Optional Debug Line
        // snprintf(buffer, sizeof(buffer),
        //          "SelfPower init: slot%d GPIO%d (%s_CH%u), gain=%.3f, %lu Hz total\n",
        //          i, S.slot[i].gpio,
        //          (S.unit == ADC_UNIT_1) ? "ADC1" : "ADC2",
        //          (unsigned)S.slot[i].ch,
        //          (double)S.slot[i].divider_gain, (unsigned long)MIN_FREQUENCY_HZ);
        // mutexPrint(buffer);
    }
}

// ============================================================================
//                                 COLLECT
// ============================================================================

void collectSelfPowerAllmV(int32_t* out_vin_mV, selfPowerStatus_t* out_statuses) {
    const int n = S.total_slots;

    // Default statuses on entry
    if (out_statuses)
        for (int i = 0; i < n; ++i) out_statuses[i] = INIT_FAILURE;
    if (!out_vin_mV || !out_statuses) return;

    if (!S.initialized) {
        // already set INIT_FAILURE
        return;
    }

    // Zero outputs
    for (int i = 0; i < n; ++i) out_vin_mV[i] = 0;

    // Per-slot accumulators sized to total slots (we’ll ignore disabled ones)
    int64_t sum[n];
    memset(sum, 0, sizeof(sum));
    int cnt[n];
    memset(cnt, 0, sizeof(cnt));

    uint32_t got = 0;

    // Drain available bytes
    for (;;) {
        esp_err_t err = adc_continuous_read(S.adc_h, s_adc_read_buf, ADC_READ_BUF_BYTES, &got, 0);
        if (err == ESP_ERR_TIMEOUT || got == 0) break;
        if (err != ESP_OK) {
            for (int i = 0; i < n; ++i) out_statuses[i] = READ_FAILURE;
            return;
        }

        for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= got; i += sizeof(adc_digi_output_data_t)) {
            const adc_digi_output_data_t* d = (const adc_digi_output_data_t*) &s_adc_read_buf[i];
            int slot = channel_to_slot(d->type1.channel);
            if (slot >= 0) {
                sum[slot] += d->type1.data;
                cnt[slot]++;
            }
        }
    }

    // Convert
    for (int i = 0; i < n; ++i) {
        if (!S.slot[i].enabled) {
            out_statuses[i] = INIT_FAILURE;
            continue;
        }
        if (cnt[i] == 0) {
            out_statuses[i] = NOTHING_TO_READ;
            continue;
        }

        const int avg_raw = (int) (sum[i] / cnt[i]);
        int pin_mV = 0;

        if (S.slot[i].cali_h) {
            if (adc_cali_raw_to_voltage(S.slot[i].cali_h, avg_raw, &pin_mV) != ESP_OK) { pin_mV = 0; }
        } else {
            const float assumed_fullscale_mV = 3300.0f;
            pin_mV = (int) ((avg_raw / 4095.0f) * assumed_fullscale_mV);
        }

        const float vin = (float) pin_mV * S.slot[i].divider_gain;
        out_vin_mV[i] = (int32_t) (vin + 0.5f);
        out_statuses[i] = READ_SUCCESS;

        // Optional debug line
        // char pb[144];
        // snprintf(pb, sizeof(pb),
        //          "SelfPower[%s slot %d]: avg=%d raw -> pin=%d mV -> Vin=%" PRId32 " mV (samples=%d)\n",
        //          (S.unit == ADC_UNIT_1) ? "ADC1" : "ADC2",
        //          i, avg_raw, pin_mV, out_vin_mV[i], cnt[i]);
        // mutexPrint(pb);
    }
}

// ============================================================================
//                         Status array helper
// ============================================================================

void selfPowerStatusCheck(const selfPowerStatus_t* statuses, int num_channels, int id) {
    if (!statuses || num_channels <= 0) return;

    bool any_init_failure = false;

    for (int i = 0; i < num_channels; ++i) {
        const selfPowerStatus_t s = statuses[i];
        if (s == NOTHING_TO_READ || s == READ_FAILURE) {
            // char buffer[64];
            // sprintf(buffer, "SelfPower channel %d read failed\n", i);
            // mutexPrint(buffer);
            sendStatusUpdate(s, id);
        }
        if (s == INIT_FAILURE) any_init_failure = true;
    }

    if (any_init_failure) { esp_restart(); }
}
