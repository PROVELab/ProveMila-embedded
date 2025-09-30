
#include <stdio.h>
#include <stdbool.h>

#include "powerSensor.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"




// ADC configuration
#define ADC_CHANNEL ADC1_CHANNEL_6 // GPIO34
#define ADC_UNIT ADC_UNIT_1

// Resistor values for the voltage divider
#define R1_VALUE 47000.0 // 47kΩ
#define R2_VALUE 10000.0 // 10kΩ

// ADC calibration value is handled by esp_adc_cal internally.
// The default of 1100mV is only used as a fallback if eFuse is not available.
// See esp_adc_cal.h for more info.

void initializeSelfPower(selfPowerConfig config){

}

void collectSelfPowermV(void) {
    static bool initializedSelfPower = false;
    if(!initializedSelfPower){

    }
    esp_adc_cal_characteristics_t adc_chars;
    
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);

    // Characterize the ADC and store the calibration data in adc_chars.
    // If eFuse data is available, it will be used for Vref.
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    // Log the calibration type to verify if eFuse is being used
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI("MAIN", "eFuse Vref: %d mV", adc_chars.vref);
    } else if (val_type == ESP_ADC_CAL_VAL_DEFAULT_VREF) {
        ESP_LOGI("MAIN", "Default Vref: %d mV", adc_chars.vref);
    }

    while (1) {
        uint32_t adc_reading = adc1_get_raw(ADC_CHANNEL);
        
        // Convert ADC reading to voltage in mV using the calibrated characteristics
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
        
        // Calculate input voltage based on the voltage divider ratio
        float v_in = voltage_mv * (R1_VALUE + R2_VALUE) / R2_VALUE;

        ESP_LOGI("MAIN", "ADC Raw: %d, Divided Voltage: %d mV, Input Voltage: %.2f V", adc_reading, voltage_mv, v_in / 1000.0);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}