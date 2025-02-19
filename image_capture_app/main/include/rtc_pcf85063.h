#ifndef RTC_PCF85063_H
#define RTC_PCF85063_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// RTC DateTime Structure
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t week;
    bool available;
} RTC_DateTime;

// Function Prototypes
esp_err_t initialize_rtc(void);
esp_err_t rtc_set_datetime(RTC_DateTime datetime);
RTC_DateTime rtc_get_datetime(void);
const char *rtc_get_formatted_date(RTC_DateTime datetime);

#endif // RTC_PCF85063_H
