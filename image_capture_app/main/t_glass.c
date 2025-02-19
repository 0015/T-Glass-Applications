#include "t_glass.h"
#include "touch_element/touch_button.h"
#include "battery_measurement.h"
#include "esp_timer.h" // For getting timestamps
#include "esp_log.h"
#include <string.h>

#define TOUCH_BUTTON_NUM 1
#define DOUBLE_TAP_THRESHOLD_MS 300 // Define a threshold for double-tap detection (e.g., 300 ms)
#define TAG "[Glass Panel]"

// Define the variables
lv_obj_t *base_ui = NULL;
lv_timer_t *base_timer = NULL;

lv_color_t font_color;
lv_color_t bg_color;
bool time_colon_state;

lv_obj_t *time_hh;
lv_obj_t *time_mm;
lv_obj_t *time_colon;
lv_obj_t *date_label;

lv_obj_t *battery_label;
lv_obj_t *ble_label;

static lv_obj_t *canvas;
static lv_color_t *canvas_buf;
static lv_image_dsc_t img_dsc;

static bool first_press = true;

static int64_t last_call_time = 0; // Stores the timestamp of the last call

bool can_execute_function()
{
    int64_t current_time = esp_timer_get_time();   // Get the current time in microseconds
    if ((current_time - last_call_time) >= 500000) // Check if 0.5 second (500,000 µs) has passed
    {
        last_call_time = current_time; // Update the last call time
        return true;                   // Function can be executed
    }
    return false; // Function should not be executed
}

/* Touch buttons handle */
static touch_button_handle_t button_handle[TOUCH_BUTTON_NUM];

/* Touch buttons channel array */
static const touch_pad_t channel_array[TOUCH_BUTTON_NUM] = {
    TOUCH_PAD_NUM14,
};

/* Touch buttons channel sensitivity array */
static const float channel_sens_array[TOUCH_BUTTON_NUM] = {
    0.02F,
};

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

esp_err_t initialize_spi_bus()
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = BOARD_DISP_SCK,
        .mosi_io_num = BOARD_DISP_MOSI,
        .miso_io_num = BOARD_DISP_MISO,
        .quadwp_io_num = BOARD_NONE_PIN,
        .quadhd_io_num = BOARD_NONE_PIN,
        //.max_transfer_sz = JD9613_HEIGHT * 80 * sizeof(uint16_t),
        .max_transfer_sz = JD9613_WIDTH * JD9613_HEIGHT * 2,
        .data4_io_num = 0,
        .data5_io_num = 0,
        .data6_io_num = 0,
        .data7_io_num = 0,
        .flags = 0x00,
        .intr_flags = 0x00,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
    };

    return spi_bus_initialize(BOARD_DISP_HOST, &buscfg, SPI_DMA_CH_AUTO);
}

esp_err_t initialize_panel_io()
{
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BOARD_DISP_DC,
        .cs_gpio_num = BOARD_DISP_CS,
        .pclk_hz = DEFAULT_SCK_SPEED,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .flags.dc_low_on_data = 0,
        .flags.octal_mode = 0,
        .flags.lsb_first = 0,
        .flags.quad_mode = 0,
        .flags.sio_mode = 0,
        .flags.cs_high_active = 0};

    // Attach the LCD to the SPI bus
    return esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BOARD_DISP_HOST, &io_config, &io_handle);
}

esp_err_t initialize_panel_jd9613()
{
    ESP_ERROR_CHECK(initialize_spi_bus());
    ESP_ERROR_CHECK(initialize_panel_io());

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_DISP_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .flags.reset_active_high = 0,
        .vendor_config = NULL};

    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9613(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    setBrightness(panel_handle, 255);

    /* Add LCD screen */
    ESP_LOGI(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = JD9613_WIDTH * JD9613_HEIGHT,
        .double_buffer = false,
        .hres = JD9613_WIDTH,
        .vres = JD9613_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .sw_rotate = true,
            .swap_bytes = false,
            .full_refresh = true,
            .direct_mode = false,
        }};

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);

    if (disp)
    {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "[Err] LVGL Display is not setup properly");
    return ESP_FAIL;
}

/* Button callback routine */
static void button_handler(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg)
{
    (void)out_handle; // Unused

    static bool single_button_press = false;

    if (out_message->event == TOUCH_BUTTON_EVT_ON_PRESS)
    {
        ESP_LOGI(TAG, "Button[%d] Press", (int)arg);

        single_button_press = true;
    }
    else if (out_message->event == TOUCH_BUTTON_EVT_ON_RELEASE)
    {
        ESP_LOGI(TAG, "Button[%d] Release", (int)arg);
        if (single_button_press)
        {
            if (first_press)
            {
                // On the first button press, go to the last tile
            }
            else
            {
                // On subsequent presses, go to the next tile and remove the previous one
            }
        }
    }
    else if (out_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS)
    {
        ESP_LOGI(TAG, "Button[%d] LongPress", (int)arg);
        single_button_press = false;
    }
}

esp_err_t initialize_touch_pad()
{
    ESP_LOGI(TAG, "Creating button\n");

    /* Initialize Touch Element library */
    touch_elem_global_config_t global_config = TOUCH_ELEM_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_element_install(&global_config));
    ESP_LOGI(TAG, "Touch element library installed");

    touch_button_global_config_t button_global_config = TOUCH_BUTTON_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_button_install(&button_global_config));
    ESP_LOGI(TAG, "Touch button installed");

    for (int i = 0; i < TOUCH_BUTTON_NUM; i++)
    {
        touch_button_config_t button_config = {
            .channel_num = channel_array[i],
            .channel_sens = channel_sens_array[i]};
        /* Create Touch buttons */
        ESP_ERROR_CHECK(touch_button_create(&button_config, &button_handle[i]));
        /* Subscribe touch button events (On Press, On Release, On LongPress) */
        ESP_ERROR_CHECK(touch_button_subscribe_event(button_handle[i],
                                                     TOUCH_ELEM_EVENT_ON_PRESS | TOUCH_ELEM_EVENT_ON_RELEASE | TOUCH_ELEM_EVENT_ON_LONGPRESS,
                                                     (void *)channel_array[i]));

        /* Set EVENT as the dispatch method */
        ESP_ERROR_CHECK(touch_button_set_dispatch_method(button_handle[i], TOUCH_ELEM_DISP_CALLBACK));
        /* Register a handler function to handle event messages */
        ESP_ERROR_CHECK(touch_button_set_callback(button_handle[i], button_handler));
        /* Set LongPress event trigger threshold time */
        ESP_ERROR_CHECK(touch_button_set_longpress(button_handle[i], 1000));
    }

    return touch_element_start();
}

esp_err_t initialize_battery_measurement()
{
    return battery_measurement_init();
}

void base_view(lv_obj_t *parent)
{
    lvgl_port_lock(0);

    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);
    // TIME
    lv_obj_t *base_tile = lv_obj_create(parent);
    lv_obj_set_size(base_tile, LV_PCT(100), LV_PCT(22));
    lv_obj_set_style_bg_color(base_tile, bg_color, 0);
    lv_obj_set_style_border_width(base_tile, 0, 0);
    lv_obj_set_scrollbar_mode(base_tile, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(base_tile, LV_DIR_NONE);

    battery_label = lv_label_create(base_tile);
    lv_obj_set_style_text_color(battery_label, font_color, 0);
    lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_10, 0);

    ble_label = lv_label_create(base_tile);
    lv_obj_set_style_text_color(ble_label, font_color, 0);
    lv_label_set_text(ble_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(ble_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_font(ble_label, &lv_font_montserrat_10, 0);


    lvgl_port_unlock();
}

const char *get_battery_icon(float voltage)
{
    if (voltage >= 4.0)
    {
        return LV_SYMBOL_BATTERY_FULL; // Full battery
    }
    else if (voltage >= 3.85)
    {
        return LV_SYMBOL_BATTERY_3; // 3 bars
    }
    else if (voltage >= 3.70)
    {
        return LV_SYMBOL_BATTERY_2; // 2 bars
    }
    else if (voltage >= 3.50)
    {
        return LV_SYMBOL_BATTERY_1; // 1 bar
    }
    else
    {
        return LV_SYMBOL_BATTERY_EMPTY; // Empty battery
    }
}

void sys_timer_fn(lv_timer_t *timer)
{
    lvgl_port_lock(0);
    float battery_voltage = battery_measurement_read();
    // Convert voltage to percentage
    int battery_percentage = battery_voltage_to_percentage(battery_voltage);
    // ESP_LOGI(TAG, "Battery Voltage: %.2f V, Battery Percentage: %d%%\n", battery_voltage, battery_percentage);

    char battery_info[100];
    snprintf(battery_info, sizeof(battery_info), "%.2fV, %d%% %s", battery_voltage, battery_percentage, get_battery_icon(battery_voltage));
    lv_label_set_text(battery_label, battery_info);
    lvgl_port_unlock();
}

void create_lv_canvas(lv_obj_t *parent)
{
    // Allocate buffer in PSRAM for RGB565 format
    canvas_buf = (lv_color_t *)heap_caps_malloc(GlassViewableWidth * GlassViewableHeight * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!canvas_buf)
    {
        ESP_LOGE(TAG, "[Err] Failed to allocate canvas buffer\n");
        return;
    }

    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.w = GlassViewableWidth;
    img_dsc.header.h = GlassViewableHeight;
    img_dsc.data = (const uint8_t *)canvas_buf;
    img_dsc.data_size = GlassViewableWidth * GlassViewableHeight * sizeof(lv_color_t);

    // Create LVGL canvas
    canvas = lv_image_create(parent);
    lv_image_set_src(canvas, &img_dsc);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
}

esp_err_t init_tglass()
{
    if (initialize_panel_jd9613() != ESP_OK)
    {
        ESP_LOGE(TAG, "[Err] JD9613 panel driver setup failed");
        return ESP_FAIL;
    }

    if (initialize_battery_measurement() != ESP_OK)
    {
        ESP_LOGE(TAG, "[Err] Battery Measurement setup failed");
        return ESP_FAIL;
    }

    if (initialize_touch_pad() != ESP_OK)
    {
        ESP_LOGE(TAG, "[Err] Touch pad setup failed");
        return ESP_FAIL;
    }

    lvgl_port_lock(0);

    font_color = lv_color_white();
    bg_color = lv_color_black();

    // Create a display base object
    base_ui = lv_tileview_create(lv_screen_active());
    lv_obj_clear_flag(base_ui, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(base_ui, lv_color_black(), 0);
    lv_obj_set_style_text_color(base_ui, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_border_width(base_ui, 0, 0);
    // lv_obj_set_pos(base_ui, 0, GlassViewable_X_Offset);
    lv_obj_set_size(base_ui, GlassViewableWidth, GlassViewableHeight);
    lv_obj_align(base_ui, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    create_lv_canvas(base_ui);

    base_view(base_ui);
    base_timer = lv_timer_create(sys_timer_fn, 1000, NULL);

    lvgl_port_unlock();
    return ESP_OK;
}

void lv_gui_ble_status(bool isOn)
{
    lvgl_port_lock(0);
    lv_obj_set_style_text_color(ble_label, isOn ? lv_palette_main(LV_PALETTE_BLUE) : font_color, 0);
    lvgl_port_unlock();
}


void update_canvas_with_rgb565(uint8_t *data, size_t len)
{
    if (!canvas_buf)
        return;
    lvgl_port_lock(0);
    memcpy(canvas_buf, data, len); // Copy BLE image data into the buffer

    // Update the image descriptor with new data
    img_dsc.data = (const uint8_t *)canvas_buf;

    // Refresh LVGL object
    lv_obj_invalidate(canvas);
    lvgl_port_unlock();
}