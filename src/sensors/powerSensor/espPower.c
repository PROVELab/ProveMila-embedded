#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define TAG "SELF_POWER"

// ---------- Your defines ----------
#define ADC_CHANNEL        ADC_CHANNEL_6        // ADC1_CHANNEL_6 == GPIO34
#define ADC_UNIT           ADC_UNIT_1

#define R1_VALUE           47000.0f             // 47kΩ (top)
#define R2_VALUE           10000.0f             // 10kΩ (bottom)
#define DIVIDER_GAIN       ((R1_VALUE + R2_VALUE) / R2_VALUE)   // ~5.7

// Sampling plan: 200 Hz -> 5 ms per sample, average ~20 each 100 ms read
#define SELF_POWER_SAMPLE_FREQ_HZ  200

// Frame/pool sizing: small but safe for low sample rate
#define ADC_CONV_FRAME_BYTES       256
#define ADC_POOL_BYTES             1024

// Buffer we read DMA results into each call
#define READ_BUF_BYTES             512

// -------- Optional: expose the latest computed mV here --------
static volatile int g_self_power_input_mV = 0;  // Vin after divider compensation
int getSelfPower_mV(void) { return g_self_power_input_mV; }

// ---------- Local state ----------
static adc_continuous_handle_t s_adc_handle = NULL;
static adc_cali_handle_t       s_adc_cali   = NULL;
static bool                    s_initialized = false;

// A tiny helper: parse one digi result
static bool parse_one_result(const adc_digi_output_data_t *d, int *raw_out) {
    // Format is ADC_DIGI_OUTPUT_FORMAT_TYPE1 by default on ESP32
    // Check channel, unit and get raw
    if (d->type1.channel == ADC_CHANNEL && d->type1.unit == ADC_UNIT) {
        *raw_out = d->type1.data;
        return true;
    }
    return false;
}

// ---- You can define the config the caller passes if you like ----
typedef struct {
    adc_atten_t atten;          // e.g. ADC_ATTEN_DB_11
    adc_bitwidth_t bitwidth;    // ADC_BITWIDTH_DEFAULT
    uint32_t sample_hz;         // e.g. 200
} selfPowerConfig;

static esp_err_t init_calibration(adc_unit_t unit, adc_channel_t chan,
                                  adc_atten_t atten, adc_bitwidth_t bitwidth)
{
    // Try Curve Fitting first (more accurate when supported), else Line Fitting
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id  = unit,
        .chan     = chan,
        .atten    = atten,
        .bitwidth = bitwidth,
    };
    if (adc_cali_create_scheme_curve_fitting(&cfg, &s_adc_cali) == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration: curve-fitting");
        return ESP_OK;
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg = {
        .unit_id  = unit,
        .atten    = atten,
        .bitwidth = bitwidth,
#if CONFIG_IDF_TARGET_ESP32
        // Only needed if eFuse Vref/TP not present:
        .default_vref = 1100, // mV fallback; used only if required by your chip
#endif
    };
    if (adc_cali_create_scheme_line_fitting(&cfg, &s_adc_cali) == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration: line-fitting");
        return ESP_OK;
    }
#endif
    ESP_LOGW(TAG, "ADC calibration not supported (no eFuse or scheme), using uncalibrated values");
    s_adc_cali = NULL;
    return ESP_ERR_NOT_SUPPORTED;
}

// ================== PUBLIC API ==================

void initializeSelfPower(selfPowerConfig config)
{
    if (s_initialized) return;

    // 1) Allocate continuous driver
    adc_continuous_handle_cfg_t hcfg = {
        .max_store_buf_size = ADC_POOL_BYTES,
        .conv_frame_size    = ADC_CONV_FRAME_BYTES,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&hcfg, &s_adc_handle));

    // 2) Configure channel pattern
    adc_digi_pattern_config_t pattern = {
        .atten     = config.atten,             // e.g. ADC_ATTEN_DB_11 (~0-3.3V at pin)
        .channel   = ADC_CHANNEL,              // ADC1_CH6
        .unit      = ADC_UNIT,                 // ADC_UNIT_1
        .bit_width = config.bitwidth,          // ADC_BITWIDTH_DEFAULT -> max for chip
    };

    adc_continuous_config_t cfg = {
        .sample_freq_hz = config.sample_hz,    // 200 Hz
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1, // single unit, ADC1
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num    = 1,
        .adc_pattern    = &pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(s_adc_handle, &cfg));

    // 3) Start
    ESP_ERROR_CHECK(adc_continuous_start(s_adc_handle));

    // 4) Calibration (uses eFuse when available)
    (void)init_calibration(ADC_UNIT, ADC_CHANNEL, config.atten, config.bitwidth);

    s_initialized = true;
    ESP_LOGI(TAG, "SelfPower ADC started @ %lu Hz on ADC1_CH%u (GPIO34)",
             (unsigned long)config.sample_hz, (unsigned)ADC_CHANNEL);
}

void collectSelfPowermV(void)
{
    if (!s_initialized) {
        selfPowerConfig def = {
            .atten     = ADC_ATTEN_DB_11,           // maximize range at pin
            .bitwidth  = ADC_BITWIDTH_DEFAULT,      // 12-bit on ESP32
            .sample_hz = SELF_POWER_SAMPLE_FREQ_HZ, // 200 Hz
        };
        initializeSelfPower(def);
    }

    uint8_t buf[READ_BUF_BYTES];
    uint32_t got = 0;
    int64_t sum_raw = 0;
    int count = 0;

    // drain up to READ_BUF_BYTES worth of results (non-blocking-ish)
    esp_err_t err = adc_continuous_read(s_adc_handle, buf, sizeof(buf), &got, /*timeout_ms*/ 0);
    if (err == ESP_OK && got > 0) {
        for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= got; i += sizeof(adc_digi_output_data_t)) {
            const adc_digi_output_data_t *d = (const adc_digi_output_data_t *)&buf[i];
            int raw = 0;
            if (parse_one_result(d, &raw)) {
                sum_raw += raw;
                count++;
            }
        }
    } else if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "adc_continuous_read err=%d", err);
    }

    if (count == 0) {
        // Nothing available this tick; keep last value
        return;
    }

    int avg_raw = (int)(sum_raw / count);

    // Convert to mV at the *pin* (after divider), using calibration if available
    int pin_mV = 0;
    if (s_adc_cali) {
        if (adc_cali_raw_to_voltage(s_adc_cali, avg_raw, &pin_mV) != ESP_OK) {
            pin_mV = 0;
        }
    } else {
        // very rough fallback if no calibration available (not recommended)
        // Assume 12-bit and 1100 mV @ 0 dB; at 11 dB full-scale ~ 3100–3600 mV depending on chip
        // You can refine if needed.
        const float assumed_fullscale_mV = 3300.0f;
        pin_mV = (int)((avg_raw / 4095.0f) * assumed_fullscale_mV);
    }

    // Scale back through divider to get *input/source* voltage
    float vin_mV_f = (float)pin_mV * DIVIDER_GAIN;
    int vin_mV = (int)(vin_mV_f + 0.5f);

    g_self_power_input_mV = vin_mV;

    // Optional: log a concise line for debugging
    ESP_LOGD(TAG, "samples=%d raw_avg=%d pin=%d mV -> Vin=%d mV", count, avg_raw, pin_mV, vin_mV);
}
