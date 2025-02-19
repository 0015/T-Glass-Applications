#pragma once
#include "jd9613.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#define BOARD_DISP_HOST     SPI3_HOST
#define BOARD_NONE_PIN      (-1)

#define BOARD_DISP_CS       (17)
#define BOARD_DISP_SCK      (15)
#define BOARD_DISP_MISO     (BOARD_NONE_PIN)
#define BOARD_DISP_MOSI     (16)
#define BOARD_DISP_DC       (21)
#define BOARD_DISP_RST      (18)

#define BOARD_I2C_SDA       (9)
#define BOARD_I2C_SCL       (8)

#define BOARD_BHI_IRQ       (37)
#define BOARD_BHI_CS        (36)
#define BOARD_BHI_SCK       (35)
#define BOARD_BHI_MISO      (34)
#define BOARD_BHI_MOSI      (33)
#define BOARD_BHI_RST       (47)
#define BOARD_BHI_EN        (48)

#define BOARD_RTC_IRQ       (7)

#define BOARD_TOUCH_BUTTON  (14)
#define BOARD_BOOT_PIN      (0)
#define BOARD_BAT_ADC       (13)
#define BOARD_VIBRATION_PIN (38)
#define DEFAULT_SCK_SPEED   (70 * 1000 * 1000)

#define BOARD_MIC_CLOCK     (6)
#define BOARD_MIC_DATA      (5)

#define GlassViewable_X_Offset          168
#define GlassViewableWidth              126
#define GlassViewableHeight             126

extern lv_obj_t *base_ui;
extern lv_timer_t *base_timer;
extern lv_color_t font_color;
extern lv_color_t bg_color;

esp_err_t init_tglass();
void lv_gui_ble_status(bool isOn);
void update_canvas_with_rgb565(uint8_t *data, size_t len);