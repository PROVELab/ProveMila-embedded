// espPower.c
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include "esp_err.h"
#include "esp_check.h"

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "../../espBase/debug_esp.h"   // mutexPrint()
#include "powerSensor.h"               // selfPowerConfig + prototypes + selfPowerStatus_t

// ---- sizing (20 kHz fixed) ----
#define MIN_FREQUENCY_HZ         20000u // SOC_ADC_SAMPLE_FREQ_THRES_LOW
#define ADC_TOTAL_SAMPLES        256u   // ~12.8 ms @ 20 kHz
#define ADC_CONV_FRAME_SAMPLES   (ADC_TOTAL_SAMPLES / 4u)  // 64 samples/frame (~3.2 ms)
#define ADC_READ_BUF_SAMPLES     (ADC_TOTAL_SAMPLES)       // drain up to full window each call

// ---- bytes ----
#define ADC_SAMPLE_BYTES         (sizeof(adc_digi_output_data_t))
#define ADC_POOL_BYTES           (ADC_TOTAL_SAMPLES      * ADC_SAMPLE_BYTES)
#define ADC_CONV_FRAME_BYTES     (ADC_CONV_FRAME_SAMPLES * ADC_SAMPLE_BYTES)
#define ADC_READ_BUF_BYTES       (ADC_READ_BUF_SAMPLES   * ADC_SAMPLE_BYTES)

_Static_assert(ADC_READ_BUF_SAMPLES >= ADC_CONV_FRAME_SAMPLES, "read buf must be >= frame");
_Static_assert((ADC_POOL_BYTES % 4) == 0, "pool must be 4-byte aligned");

// Use DB_12 attenuation and 12-bit width
static const adc_atten_t    defaultAtten    = ADC_ATTEN_DB_12;
static const adc_bitwidth_t defaultBitWidth = ADC_BITWIDTH_12;

// -------- Module state --------
static struct {
    bool initialized;
    selfPowerConfig hw;               // from powerSensor.h
    float divider_gain;               // (R1+R2)/R2
    adc_continuous_handle_t adc_h;
    adc_cali_handle_t cali_h;
    adc_unit_t unit;                  // ADC_UNIT_1
    adc_channel_t channel;            // resolved from ADCPin
} S = {0};

// DMA-capable buffer, for reading measurements (app-owned)
DMA_ATTR static uint8_t s_adc_read_buf[ADC_READ_BUF_BYTES];

// -------- GPIO→ADC1 channel map (ESP32) --------
static bool gpio_to_adc1_channel(int gpio, adc_channel_t *ch_out) {
    switch (gpio) {
        case 36: *ch_out = ADC_CHANNEL_0; return true;
        case 37: *ch_out = ADC_CHANNEL_1; return true;
        case 38: *ch_out = ADC_CHANNEL_2; return true;
        case 39: *ch_out = ADC_CHANNEL_3; return true;
        case 32: *ch_out = ADC_CHANNEL_4; return true;
        case 33: *ch_out = ADC_CHANNEL_5; return true;
        case 34: *ch_out = ADC_CHANNEL_6; return true;
        case 35: *ch_out = ADC_CHANNEL_7; return true;
        default: return false;
    }
}

// calibration. returns true on success, false on failure (non-fatal)
static bool init_calibration(adc_unit_t unit, adc_channel_t ch) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg_cf = {
        .unit_id  = unit,
        .chan     = ch,
        .atten    = defaultAtten,
        .bitwidth = defaultBitWidth,
    };
    if (adc_cali_create_scheme_curve_fitting(&cfg_cf, &S.cali_h) == ESP_OK) {
        mutexPrint("ADC calibration: curve-fitting enabled\n");
        return true;
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg_lf = {
        .unit_id  = unit,
        .atten    = defaultAtten,
        .bitwidth = defaultBitWidth,
    #if CONFIG_IDF_TARGET_ESP32
        .default_vref = 1100, // mV fallback if no eFuse data
    #endif
    };
    if (adc_cali_create_scheme_line_fitting(&cfg_lf, &S.cali_h) == ESP_OK) {
        mutexPrint("ADC calibration: line-fitting enabled\n");
        return true;
    }
#endif
    mutexPrint("ADC calibration: unavailable, running uncalibrated\n");
    S.cali_h = NULL;
    return false;
}

// Parse one piece of data from the DMA buffer
static inline bool parse_one(const adc_digi_output_data_t *d, int *raw_out) {
    if (d->type1.channel == S.channel) {
        *raw_out = d->type1.data;
        return true;
    }
    return false;
}

// -------- PUBLIC API --------
// Setup ADC and DMA. Returns selfPowerStatus_t per header enum.
selfPowerStatus_t initializeSelfPower(selfPowerConfig config) {
    if (S.initialized) {
        mutexPrint("SelfPower already initialized\n");
        return (S.cali_h ? SUCCESSFUL_INIT_CALIBRATED : SUCCESSFUL_INIT_NO_CALIBRATION);
    }

    if (config.ADCUnit != 1) {
        mutexPrint("Only ADC1 is supported (ADCUnit=1)\n");
        return INIT_FAILURE;
    }
    if (config.R1 <= 0 || config.R2 <= 0) {
        mutexPrint("Invalid R1/R2 divider values\n");
        return INIT_FAILURE;
    }
    if (!gpio_to_adc1_channel(config.ADCPin, &S.channel)) {
        mutexPrint("ADC GPIO PIN invalid (must be GPIO32–39)\n");
        return INIT_FAILURE;
    }

    S.hw = config;
    S.unit = ADC_UNIT_1;
    S.divider_gain = ((float)config.R1 + (float)config.R2) / (float)config.R2;

    // 1) Handle (driver ring and frame sizes)
    adc_continuous_handle_cfg_t hcfg = {
        .max_store_buf_size = ADC_POOL_BYTES,
        .conv_frame_size    = ADC_CONV_FRAME_BYTES,
    };
    if (adc_continuous_new_handle(&hcfg, &S.adc_h) != ESP_OK) {
        mutexPrint("adc_continuous_new_handle failed\n");
        return INIT_FAILURE;
    }

    // 2) Pattern
    adc_digi_pattern_config_t pattern = {
        .atten     = defaultAtten,
        .channel   = S.channel,
        .unit      = S.unit,    // harmless for single-unit targets
        .bit_width = defaultBitWidth,
    };

    // 3) Fixed minimum sampling rate
    adc_continuous_config_t cfg = {
        .sample_freq_hz = MIN_FREQUENCY_HZ,               // 20 kHz
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num    = 1,
        .adc_pattern    = &pattern,
    };
    if (adc_continuous_config(S.adc_h, &cfg) != ESP_OK) {
        mutexPrint("adc_continuous_config failed\n");
        return INIT_FAILURE;
    }
    if (adc_continuous_start(S.adc_h) != ESP_OK) {
        mutexPrint("adc_continuous_start failed\n");
        return INIT_FAILURE;
    }

    bool calibrated = init_calibration(S.unit, S.channel);
    S.initialized = true;

    char buffer[160];
    snprintf(buffer, sizeof(buffer),
             "SelfPower init: GPIO%d (ADC1_CH%u), %d/%dΩ, gain=%.3f, %lu Hz fixed (~50 µs/sample)\n",
             config.ADCPin, (unsigned)S.channel, config.R1, config.R2,
             (double)S.divider_gain, (unsigned long)MIN_FREQUENCY_HZ);
    mutexPrint(buffer);

    return calibrated ? SUCCESSFUL_INIT_CALIBRATED : SUCCESSFUL_INIT_NO_CALIBRATION;
}

// Read and compute Vin (mV). Returns status, writes result to *out_vin_mV on READ_SUCCESS.
selfPowerStatus_t collectSelfPowermV(int32_t *out_vin_mV) {
    if (!S.initialized) {
        mutexPrint("collectSelfPowermV() called before initializeSelfPower()\n");
        return INIT_FAILURE;
    }
    if (!out_vin_mV) {
        mutexPrint("collectSelfPowermV() out pointer is NULL\n");
        return READ_FAILURE;
    }

    uint32_t got = 0;
    int64_t sum = 0;
    int n = 0;

    // Drain all currently available bytes from the driver ring
    for (;;) {
        esp_err_t err = adc_continuous_read(S.adc_h, s_adc_read_buf, ADC_READ_BUF_BYTES, &got, 0);
        if (err == ESP_ERR_TIMEOUT || got == 0) break; // nothing left at this instant
        if (err != ESP_OK) {
            char buf[64];
            snprintf(buf, sizeof(buf), "adc_continuous_read error=%d\n", (int)err);
            mutexPrint(buf);
            return READ_FAILURE;
        }

        for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= got; i += sizeof(adc_digi_output_data_t)) {
            const adc_digi_output_data_t *d = (const adc_digi_output_data_t *)&s_adc_read_buf[i];
            int raw;
            if (parse_one(d, &raw)) { sum += raw; n++; }
        }
    }

    if (n == 0) {
        return NOTHING_TO_READ;
    }

    const int avg_raw = (int)(sum / n);
    int pin_mV = 0;

    if (S.cali_h) {
        if (adc_cali_raw_to_voltage(S.cali_h, avg_raw, &pin_mV) != ESP_OK) {
            pin_mV = 0; // graceful fallback
        }
    } else {
        // Uncalibrated estimate
        const float assumed_fullscale_mV = 3300.0f;
        pin_mV = (int)((avg_raw / 4095.0f) * assumed_fullscale_mV);
    }

    const float vin = (float)pin_mV * S.divider_gain;
    *out_vin_mV = (int32_t)(vin + 0.5f);

    // Debug line (optional—keep or remove)
    char printBuffer[128];
    snprintf(printBuffer, sizeof(printBuffer),
             "SelfPower: ADC avg=%d raw -> pin=%d mV -> Vin=%" PRId32 " mV (samples=%d)\n",
             avg_raw, pin_mV, *out_vin_mV, n);
    mutexPrint(printBuffer);

    return READ_SUCCESS;
}

//checks for errors. If theres an error, sends CAN status update, and aborts.
void selfPowerStatusCheck(selfPowerStatus_t ADC_status, int id){
    if(ADC_status != READ_SUCCESS){ //we have an error, or want to report init status
        sendStatusUpdate(ADC_status, id);
    }
    if(ADC_status == INIT_FAILURE){ //try again if we fail to initialize
        esp_restart();    
    }
}