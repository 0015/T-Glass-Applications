#ifndef BATTERY_MEASUREMENT_H
#define BATTERY_MEASUREMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "esp_check.h"

// Initializes the ADC for battery measurement
esp_err_t battery_measurement_init(void);

// Reads the battery voltage and returns the value in volts
float battery_measurement_read(void);

// Converts the battery voltage to a percentage
int battery_voltage_to_percentage(float voltage);

// Cleans up resources used for battery measurement
void battery_measurement_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_MEASUREMENT_H