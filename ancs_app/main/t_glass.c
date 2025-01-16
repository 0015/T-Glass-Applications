#include "t_glass.h"
#include "touch_element/touch_button.h"
#include "battery_measurement.h"
#include "esp_timer.h" // For getting timestamps
#include "esp_log.h"
#include "ancs_app.h"

#define TOUCH_BUTTON_NUM 1
#define DOUBLE_TAP_THRESHOLD_MS 300 // Define a threshold for double-tap detection (e.g., 300 ms)
#define TAG "[Glass Panel]"

static void lv_gui_goto_last_tile();
static void lv_gui_goto_next_tile();

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
lv_obj_t *inbox_label;

static int current_tile_index = 0; // Tracks the active tile index
static int last_tile_index = 0;    // Tracks the last added tile index
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
                lv_gui_goto_last_tile();
            }
            else
            {
                // On subsequent presses, go to the next tile and remove the previous one
                lv_gui_goto_next_tile();
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

void base_tileview(lv_obj_t *parent)
{
    lvgl_port_lock(0);

    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);
    // TIME
    lv_obj_t *base_tile = lv_obj_create(parent);
    lv_obj_set_size(base_tile, LV_PCT(100), LV_PCT(100));
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
    lv_obj_set_style_text_font(ble_label, &lv_font_montserrat_14, 0);

    inbox_label = lv_label_create(base_tile);
    lv_obj_set_style_text_color(inbox_label, font_color, 0);
    lv_obj_set_style_text_align(inbox_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(inbox_label, "Are you sure\nthere are\nno messages?");
    lv_obj_center(inbox_label);
    lv_obj_set_style_text_font(inbox_label, &lv_font_montserrat_14, 0);

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

    font_color = lv_color_white();
    bg_color = lv_color_black();

    lvgl_port_lock(0);
    // Create a display base object
    base_ui = lv_tileview_create(lv_screen_active());
    lv_obj_clear_flag(base_ui, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(base_ui, lv_color_black(), 0);
    lv_obj_set_style_text_color(base_ui, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_border_width(base_ui, 0, 0);
    // lv_obj_set_pos(base_ui, 0, GlassViewable_X_Offset);
    lv_obj_set_size(base_ui, GlassViewableWidth, GlassViewableHeight);
    lv_obj_align(base_ui, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create the displayed UI
    lv_obj_t *t1 = lv_tileview_add_tile(base_ui, 0, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    // Create date page
    base_tileview(t1);
    base_timer = lv_timer_create(sys_timer_fn, 1000, NULL);
    lvgl_port_unlock();
    return ESP_OK;
}

void add_tile_view(int index, NotificationAttributes *notification)
{
    lvgl_port_lock(0);

    if (++last_tile_index >= (MAX_NOTIFICATIONS + 1))
    {
        ESP_LOGW(TAG, "add_tile_view ignored: MAX_NOTIFICATIONS reached.");
        return;
    }
    lv_obj_t *tile = lv_tileview_add_tile(base_ui, last_tile_index, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    ESP_LOGI(TAG, "[AA] add_tile_view(last_tile_index) : %d", (int)last_tile_index);

    lv_obj_set_scroll_dir(tile, LV_DIR_NONE);
    lv_obj_t *base_tile = lv_obj_create(tile);
    lv_obj_set_size(base_tile, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(base_tile, bg_color, 0);
    lv_obj_set_style_border_width(base_tile, 0, 0);
    lv_obj_set_scrollbar_mode(base_tile, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(base_tile, LV_DIR_NONE);

    lv_obj_t *title = lv_label_create(base_tile);
    lv_obj_set_style_text_color(title, font_color, 0);
    lv_label_set_text(title, notification->Title);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 8);

    lv_obj_t *msg = lv_label_create(base_tile);
    lv_obj_set_style_text_color(msg, font_color, 0);
    lv_obj_set_size(msg, 100, 100);
    lv_label_set_text(msg, notification->Message);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_12, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_LEFT, 4, 24);

    static lv_point_precise_t line_points[] = {{4, 20}, {100, 20}};
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&style_line, true);

    /*Create a line and apply the new style*/
    lv_obj_t *line = lv_line_create(base_tile);
    lv_line_set_points(line, line_points, 2); /*Set the points*/
    lv_obj_add_style(line, &style_line, 0);
    lv_obj_align_to(line, msg, LV_ALIGN_OUT_TOP_MID, 0, 0);

    lv_gui_set_inbox_title(last_tile_index);

    lvgl_port_unlock();
}

static void lv_gui_goto_last_tile()
{
    ESP_LOGI(TAG, "lv_gui_goto_last_tile: %d", (int)last_tile_index);
    if (last_tile_index > 0)
    {
        current_tile_index = last_tile_index; // Set to the last tile
        lv_obj_set_tile_id(base_ui, current_tile_index, 0, LV_ANIM_ON);
        ESP_LOGI(TAG, "Moved to last tile: %d", current_tile_index);
        first_press = false;
    }
    else
    {
        ESP_LOGI(TAG, "No notification tiles available.");
    }
}

static void lv_gui_goto_next_tile()
{
    ESP_LOGI(TAG, "Current tile index before navigation: %d", current_tile_index);

    // Delete the current tile if it exists
    lv_obj_t *current_tile = lv_tileview_get_tile_active(base_ui);
    if (current_tile != NULL && current_tile_index > 0)
    {
        lv_obj_delete(current_tile);
        ESP_LOGI(TAG, "Deleted tile at index: %d", current_tile_index);
        --notification_index;
        --last_tile_index; // Update the last tile index
        lv_gui_set_inbox_title(last_tile_index);
    }

    // Move to the previous tile
    if (last_tile_index > 0)
    {
        --current_tile_index; // Decrement the current index
        lv_obj_set_tile_id(base_ui, current_tile_index, 0, LV_ANIM_ON);
        ESP_LOGI(TAG, "Moved to tile: %d", current_tile_index);
    }
    else
    {
        // If no tiles are left, go back to the base tile
        current_tile_index = 0;
        lv_obj_set_tile_id(base_ui, current_tile_index, 0, LV_ANIM_ON);
        ESP_LOGI(TAG, "No more tiles. Returning to base.");

        first_press = true;
    }
}

void lv_gui_remove_all_tiles()
{
    ESP_LOGI(TAG, "Removing all tiles...");

    // Lock the LVGL port to ensure thread safety
    lvgl_port_lock(0);

    // Remove all tiles from last to first
    while (last_tile_index > 0)
    {
        lv_obj_t *current_tile = lv_tileview_get_tile_active(base_ui);
        if (current_tile != NULL && last_tile_index > 0)
        {
            lv_obj_delete(current_tile);
            ESP_LOGI(TAG, "Deleted tile at index: %d", last_tile_index);
        }

        --last_tile_index;    // Decrease the last tile index
        --notification_index; // Update the notification index
    }

    // Reset indices and return to the base tile
    current_tile_index = 0;
    lv_obj_set_tile_id(base_ui, current_tile_index, 0, LV_ANIM_OFF);

    // Unlock the LVGL port
    lvgl_port_unlock();

    ESP_LOGI(TAG, "All tiles removed. Returning to base.");
}

void lv_gui_ble_status(bool isOn)
{
    lvgl_port_lock(0);
    lv_obj_set_style_text_color(ble_label, isOn ? lv_palette_main(LV_PALETTE_BLUE) : font_color, 0);

    if (!isOn)
    {
        lv_gui_remove_all_tiles();
    }
    lvgl_port_unlock();
}

void lv_gui_set_inbox_title(int notification_count)
{
    lvgl_port_lock(0);
    char buffer[128];

    if (notification_count == 0)
    {
        lv_label_set_text(inbox_label, "Hooray!\nNo Unread\nMessages!\nTime to relax.");
    }
    else if (notification_count == 1)
    {
        lv_label_set_text(inbox_label, "Just 1\nNew Message.\nMake it count!");
    }
    else if (notification_count == 2)
    {
        lv_label_set_text(inbox_label, "Two's company:\nYou have\n2 Messages!");
    }
    else if (notification_count == 3)
    {
        lv_label_set_text(inbox_label, "Three's a crowd:\nCheck your\n3 Messages!");
    }
    else if (notification_count == 4)
    {
        lv_label_set_text(inbox_label, "4 Messages\nWaiting for you.\nLet's get reading!");
    }
    else if (notification_count <= 6)
    {
        snprintf(buffer, sizeof(buffer), "Wow, %d Messages!\nYou're popular!", notification_count);
        lv_label_set_text(inbox_label, buffer);
    }
    else if (notification_count <= 9)
    {
        snprintf(buffer, sizeof(buffer), "%d Messages!\nYou're on fire!", notification_count);
        lv_label_set_text(inbox_label, buffer);
    }
    else if (notification_count <= 10)
    {
        lv_label_set_text(inbox_label, "10 Messages!\nOverflow alert!");
    }
    lvgl_port_unlock();
}