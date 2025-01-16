#include "esp_check.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "jd9613.h"

#define TAG "jd9613"

typedef struct
{
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    uint8_t rotation;
    uint16_t *frame_buffer;
    uint16_t width;
    uint16_t height;
    bool flipHorizontal;
} jd9613_panel_t;

// There is only 1/2 RAM inside the JD9613 screen, and it cannot be rotated in directions 1 and 3.
esp_err_t panel_jd9613_set_rotation(esp_lcd_panel_t *panel, uint8_t r)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;
    uint8_t write_data = 0x00;
    switch (r)
    {
    case 1: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MX_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // jd9613->width = JD9613_HEIGHT;
        // jd9613->height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    case 2:
        write_data = LCD_CMD_MY_BIT | LCD_CMD_MX_BIT | LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    case 3: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // jd9613->width = JD9613_HEIGHT;
        // jd9613->height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    default: // case 0:
        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    }

    if (jd9613->flipHorizontal)
    {
        write_data |= (0x01 << 1); // Flip Horizontal
    }
    // write_data |= 0x01; //Flip Vertical
    jd9613->rotation = r;
    ESP_LOGI(TAG, "set_rotation:%d write reg :0x%X , data : 0x%X Width:%d Height:%d", r, LCD_CMD_MADCTL, write_data, jd9613->width, jd9613->height);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &write_data, 1);
    return ESP_OK;
}

static esp_err_t panel_jd9613_del(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);

    if (jd9613->reset_gpio_num >= 0)
    {
        gpio_reset_pin(jd9613->reset_gpio_num);
    }
    ESP_LOGI(TAG, "del jd9613 panel @%p", jd9613);
    free(jd9613->frame_buffer);
    free(jd9613);
    return ESP_OK;
}

static esp_err_t panel_jd9613_reset(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;

    // perform hardware reset
    if (jd9613->reset_gpio_num >= 0)
    {
        gpio_set_level(jd9613->reset_gpio_num, jd9613->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(jd9613->reset_gpio_num, !jd9613->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    { // perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5ms before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_jd9613_init(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    int cmd = 0;
    while (jd9613_cmd[cmd].len != 0xff)
    {
        esp_lcd_panel_io_tx_param(io, jd9613_cmd[cmd].addr, jd9613_cmd[cmd].param, (jd9613_cmd[cmd].len - 1) & 0x1F);
        cmd++;
    }

    jd9613->flipHorizontal = 0;
    jd9613->rotation = 0;

    panel_jd9613_set_rotation(panel, jd9613->rotation);

    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    return ESP_OK;
}

void rgb565_swap(uint16_t *color_data, uint32_t width, uint32_t height)
{
    uint32_t total_pixels = width * height;

    uint32_t u32_cnt = total_pixels / 2;
    uint16_t *buf16 = color_data;
    uint32_t *buf32 = (uint32_t *)color_data;

    while (u32_cnt >= 8)
    {
        buf32[0] = ((buf32[0] & 0xff00ff00) >> 8) | ((buf32[0] & 0x00ff00ff) << 8);
        buf32[1] = ((buf32[1] & 0xff00ff00) >> 8) | ((buf32[1] & 0x00ff00ff) << 8);
        buf32[2] = ((buf32[2] & 0xff00ff00) >> 8) | ((buf32[2] & 0x00ff00ff) << 8);
        buf32[3] = ((buf32[3] & 0xff00ff00) >> 8) | ((buf32[3] & 0x00ff00ff) << 8);
        buf32[4] = ((buf32[4] & 0xff00ff00) >> 8) | ((buf32[4] & 0x00ff00ff) << 8);
        buf32[5] = ((buf32[5] & 0xff00ff00) >> 8) | ((buf32[5] & 0x00ff00ff) << 8);
        buf32[6] = ((buf32[6] & 0xff00ff00) >> 8) | ((buf32[6] & 0x00ff00ff) << 8);
        buf32[7] = ((buf32[7] & 0xff00ff00) >> 8) | ((buf32[7] & 0x00ff00ff) << 8);
        buf32 += 8;
        u32_cnt -= 8;
    }

    while (u32_cnt)
    {
        *buf32 = ((*buf32 & 0xff00ff00) >> 8) | ((*buf32 & 0x00ff00ff) << 8);
        buf32++;
        u32_cnt--;
    }

    if (total_pixels & 0x1)
    { // Odd pixel count, swap the last pixel separately
        uint32_t e = total_pixels - 1;
        buf16[e] = ((buf16[e] & 0xff00) >> 8) | ((buf16[e] & 0x00ff) << 8);
    }
}

static esp_err_t panel_jd9613_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = jd9613->io;

    // Swap the 2 bytes of RGB565 color
    // Cannot find a way to use it in ESP_LVGL_PORT
    rgb565_swap((uint16_t *)color_data, x_end - x_start + 1, y_end - y_start + 1);

    uint32_t width = x_start + x_end;
    uint32_t height = y_start + y_end;
    uint32_t _x = x_start,
             _y = y_start,
             _xe = width,
             _ye = height;
    size_t write_colors_bytes = width * height * sizeof(uint16_t);
    uint16_t *data_ptr = (uint16_t *)color_data;

    bool sw_rotation = false;
    switch (jd9613->rotation)
    {
    case 1:
    case 3:
        sw_rotation = true;
        break;
    default:
        sw_rotation = false;
        break;
    }

    if (sw_rotation)
    {
        _x = JD9613_WIDTH - (y_start + height);
        _y = x_start;
        _xe = height;
        _ye = width;
    }

    // Direction 2 requires offset pixels
    if (jd9613->rotation == 2)
    {
        _x += 2;
        _xe += 2;
    }

    // define an area of frame memory where MCU can access
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]){
                                                                         (_x >> 8) & 0xFF,
                                                                         _x & 0xFF,
                                                                         ((_xe - 1) >> 8) & 0xFF,
                                                                         (_xe - 1) & 0xFF,
                                                                     },
                                                  4),
                        TAG, "send command failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]){
                                                                         (_y >> 8) & 0xFF,
                                                                         _y & 0xFF,
                                                                         ((_ye - 1) >> 8) & 0xFF,
                                                                         (_ye - 1) & 0xFF,
                                                                     },
                                                  4),
                        TAG, "send command failed");

    if (sw_rotation)
    {
        int index = 0;
        uint16_t *pdat = (uint16_t *)color_data;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < height; i++)
            {
                jd9613->frame_buffer[index++] = pdat[width * (height - i - 1) + j];
            }
        }
        data_ptr = jd9613->frame_buffer;
    }
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, data_ptr, write_colors_bytes);
    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_jd9613(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK;
    jd9613_panel_t *jd9613 = NULL;
    gpio_config_t io_conf = {0};

    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

    jd9613 = (jd9613_panel_t *)calloc(1, sizeof(jd9613_panel_t));

    ESP_GOTO_ON_FALSE(jd9613, ESP_ERR_NO_MEM, err, TAG, "no mem for jd9613 panel");

    jd9613->frame_buffer = (uint16_t *)heap_caps_malloc(JD9613_WIDTH * JD9613_HEIGHT * 2, MALLOC_CAP_DMA);
    if (!jd9613->frame_buffer)
    {
        free(jd9613);
        return ESP_FAIL;
    }

    if (panel_dev_config->reset_gpio_num >= 0)
    {
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num;
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->bits_per_pixel)
    {
    case 16: // RGB565
        // fb_bits_per_pixel = 16;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    jd9613->io = io;
    // jd9613->fb_bits_per_pixel = fb_bits_per_pixel;
    jd9613->reset_gpio_num = panel_dev_config->reset_gpio_num;
    jd9613->reset_level = panel_dev_config->flags.reset_active_high;
    jd9613->base.del = panel_jd9613_del;
    jd9613->base.reset = panel_jd9613_reset;
    jd9613->base.init = panel_jd9613_init;
    jd9613->base.draw_bitmap = panel_jd9613_draw_bitmap;
    jd9613->base.invert_color = NULL;
    jd9613->base.set_gap = NULL;
    jd9613->base.mirror = NULL;
    jd9613->base.swap_xy = NULL;
    jd9613->base.disp_on_off = NULL;

    *ret_panel = &(jd9613->base);
    ESP_LOGI(TAG, "new jd9613 panel @%p", jd9613);

    return ESP_OK;

err:
    if (jd9613)
    {
        if (panel_dev_config->reset_gpio_num >= 0)
        {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(jd9613);
    }
    return ret;
}

void flipHorizontal(esp_lcd_panel_t *panel, bool enable)
{
    assert(panel);
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    jd9613->flipHorizontal = enable;
    panel_jd9613_set_rotation(panel, jd9613->rotation);
}

void writeCommand(esp_lcd_panel_t *panel, uint32_t cmd, uint8_t *pdat, uint32_t length)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_tx_param(jd9613->io, cmd, pdat, length);
}

void setBrightness(esp_lcd_panel_t *panel, uint8_t level)
{
    lcd_cmd_t t = {0x51, {level}, 1};
    writeCommand(panel, t.addr, t.param, t.len);
}