#include "rtc_pcf85063.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define I2C_MASTER_SCL_IO 8
#define I2C_MASTER_SDA_IO 9
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define PCF85063_ADDR 0x51

#define PCF85063_CTRL1_REG 0x00
#define PCF85063_SEC_REG 0x04

#define DEC2BCD(val) (((val) / 10) << 4 | ((val) % 10))
#define BCD2DEC(val) (((val) >> 4) * 10 + ((val) & 0x0F))

static const char *TAG = "RTC_PCF85063";

// Array of day names
static const char *DAY_NAMES[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Array of month abbreviations
static const char *MONTH_NAMES[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Static buffer for formatted date
static char formatted_date[20];

static esp_err_t write_register(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t read_register(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t initialize_rtc(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t rtc_set_datetime(RTC_DateTime datetime)
{
    uint8_t buffer[7];
    buffer[0] = DEC2BCD(datetime.second) & 0x7F;
    buffer[1] = DEC2BCD(datetime.minute);
    buffer[2] = DEC2BCD(datetime.hour);
    buffer[3] = DEC2BCD(datetime.day);
    buffer[4] = datetime.week;
    buffer[5] = DEC2BCD(datetime.month);
    buffer[6] = DEC2BCD(datetime.year % 100);

    esp_err_t err = write_register(PCF85063_SEC_REG, buffer, 7);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "RTC Date-Time set: %04d-%02d-%02d %02d:%02d:%02d",
                 datetime.year, datetime.month, datetime.day,
                 datetime.hour, datetime.minute, datetime.second);
    }
    return err;
}

RTC_DateTime rtc_get_datetime(void)
{
    RTC_DateTime datetime;
    uint8_t buffer[7] = {0};
    esp_err_t err = read_register(PCF85063_SEC_REG, buffer, 7);
    if (err == ESP_OK)
    {
        datetime.available = (buffer[0] & 0x80) == 0x00;
        datetime.second = BCD2DEC(buffer[0] & 0x7F);
        datetime.minute = BCD2DEC(buffer[1] & 0x7F);
        datetime.hour = BCD2DEC(buffer[2] & 0x3F);
        datetime.day = BCD2DEC(buffer[3] & 0x3F);
        datetime.week = BCD2DEC(buffer[4] & 0x07);
        datetime.month = BCD2DEC(buffer[5] & 0x1F);
        datetime.year = BCD2DEC(buffer[6]) + 2000;

        // ESP_LOGI(TAG, "RTC Date-Time retrieved: %04d-%02d-%02d %02d:%02d:%02d",
        //          datetime.year, datetime.month, datetime.day,
        //          datetime.hour, datetime.minute, datetime.second);
    }
    return datetime;
}

const char *rtc_get_formatted_date(RTC_DateTime datetime)
{
    // Validate week and month values to avoid buffer overflows
    if (datetime.week > 6 || datetime.month > 12 || datetime.month == 0)
    {
        return "Invalid Date";
    }

    snprintf(formatted_date, sizeof(formatted_date), "%s, %s %d",
             DAY_NAMES[datetime.week],
             MONTH_NAMES[datetime.month - 1],
             datetime.day);

    return formatted_date;
}