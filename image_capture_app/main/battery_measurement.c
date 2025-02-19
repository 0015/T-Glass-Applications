#include "battery_measurement.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL         ADC_CHANNEL_2 
#define ADC_UNIT            ADC_UNIT_2
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_12
#define REF_VOLTAGE         1100             // Reference voltage in mV (adjust based on ESP32's factory calibration)
#define DIVIDER_RATIO       2.0              // Resistor divider ratio (adjust based on your resistor values)

static const char *TAG = "BATTERY_MEASUREMENT";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool calibration_enabled = false;

// Battery voltage-to-percentage lookup table
typedef struct {
    float voltage;  // Battery voltage in volts
    int percentage; // Corresponding battery percentage
} battery_level_t;

// Battery voltage-to-percentage lookup table
const battery_level_t battery_curve[] = {
    {4.20, 100},
    {4.00, 90},
    {3.85, 75},
    {3.70, 50},
    {3.60, 25},
    {3.50, 10},
    {3.30, 0},
};

#define NUM_POINTS (sizeof(battery_curve) / sizeof(battery_curve[0]))

// Helper function to get battery percentage from voltage
int battery_voltage_to_percentage(float voltage) {
    if (voltage >= battery_curve[0].voltage) {
        return 100;  // Above the max voltage, assume full charge
    }
    if (voltage <= battery_curve[NUM_POINTS - 1].voltage) {
        return 0;    // Below the minimum voltage, assume empty
    }

    // Linear interpolation between points
    for (int i = 0; i < NUM_POINTS - 1; i++) {
        if (voltage <= battery_curve[i].voltage && voltage > battery_curve[i + 1].voltage) {
            float v1 = battery_curve[i].voltage;
            float v2 = battery_curve[i + 1].voltage;
            int p1 = battery_curve[i].percentage;
            int p2 = battery_curve[i + 1].percentage;

            // Linear interpolation formula
            return p1 + (voltage - v1) * (p2 - p1) / (v2 - v1);
        }
    }

    return 0;  // Default, should never reach here
}

esp_err_t battery_measurement_init(void) {
    esp_err_t ret;
    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &channel_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc_handle); // Cleanup in case of failure
        return ret;
    }

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .chan = (adc_channel_t)ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) == ESP_OK) {
        calibration_enabled = true;
        ESP_LOGI(TAG, "Calibration enabled using curve fitting");
    } else {
        ESP_LOGW(TAG, "Calibration not supported on this device. Proceeding without calibration");
    }

    ESP_LOGI(TAG, "ADC initialization successful");
    return ESP_OK;
}

float battery_measurement_read(void) {
    int raw_reading = 0;
    int voltage = 0;

    // Read raw ADC value
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_reading));

    if (calibration_enabled) {
        // Convert raw value to voltage using calibration
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_reading, &voltage));
    } else {
        // Approximate voltage calculation
        voltage = (raw_reading * REF_VOLTAGE) / (1 << ADC_BITWIDTH);
    }

    float battery_voltage = voltage * DIVIDER_RATIO / 1000.0;  // Calculate battery voltage in volts
    //ESP_LOGI(TAG, "Raw ADC Value: %d, Voltage: %d mV, Battery Voltage: %.2f V", raw_reading, voltage, battery_voltage);

    return battery_voltage;
}

void battery_measurement_deinit(void) {
    if (calibration_enabled) {
        adc_cali_delete_scheme_curve_fitting(cali_handle);
    }
    if (adc_handle != NULL) {
        adc_oneshot_del_unit(adc_handle);
    }
}